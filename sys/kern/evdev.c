#include <dev/evdev.h>
#include <sys/devfs.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/devfs.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <bitstring.h>

#define NAMELEN 80

/* evdev clock IDs in Linux semantic */
typedef enum {
  EV_CLOCK_REALTIME = 0, /* UTC clock */
  EV_CLOCK_MONOTONIC,    /* monotonic, stops on suspend */
  EV_CLOCK_BOOTTIME      /* monotonic, suspend-awared */
} evdev_clock_id_t;

static devfs_node_t *evdev_input_dir;

/*
 * Locking.
 *
 * Internal evdev structures are protected with next locks:
 * State lock		(s) - Internal state. The data it protects is changed
 *			      by incoming evdev events and some ioctls.
 * Client queue locks	(q) - One lock per client to serialize access to data
 *			      available through character device node.
 */

typedef struct evdev_dev {
  char ev_name[NAMELEN];
  struct input_id ev_id;
  int ev_unit;

  mtx_t ev_lock; /* State lock */

  /* Supported features: */
  bitstr_t bit_decl(ev_type_flags, EV_CNT);
  bitstr_t bit_decl(ev_key_flags, KEY_CNT);
  bitstr_t bit_decl(ev_flags, EVDEV_FLAG_CNT);

  /* Repeat parameters & callout: */
  int ev_rep[REP_CNT];      /* (s) */
  callout_t ev_rep_callout; /* (s) */
  uint16_t ev_rep_key;      /* (s) */

  /* State: */
  bitstr_t bit_decl(ev_key_states, KEY_CNT); /* (s) */

  LIST_HEAD(, evdev_client) ev_clients; /* (s) */
} evdev_dev_t;

typedef struct evdev_client {
  evdev_dev_t *ec_evdev;
  evdev_clock_id_t ec_clock_id;

  LIST_ENTRY(evdev_client) ec_link; /* (s) */

  mtx_t ec_buffer_mtx;            /* Client queue lock */
  condvar_t ec_buffer_cv;         /* (q) */
  size_t ec_buffer_size;          /* (q) */
  size_t ec_buffer_ready;         /* (q) */
  size_t ec_buffer_head;          /* (q) */
  size_t ec_buffer_tail;          /* (q) */
  struct input_event ec_buffer[]; /* (q) */
} evdev_client_t;

static bool evdev_event_supported(struct evdev_dev *evdev, uint16_t type) {
  assert(type < EV_CNT);
  return bit_test(evdev->ev_type_flags, type);
}

static void evdev_set_repeat_params(struct evdev_dev *evdev, uint16_t property,
                                    int value) {
  assert(property < REP_CNT);
  evdev->ev_rep[property] = value;
}

static bool evdev_client_emptyq(evdev_client_t *client) {
  return client->ec_buffer_head == client->ec_buffer_ready;
}

static void evdev_send_event(evdev_dev_t *evdev, uint16_t type, uint16_t code,
                             int32_t value);
static void evdev_register_client(evdev_dev_t *evdev, evdev_client_t *client);
static void evdev_dispose_client(evdev_dev_t *evdev, evdev_client_t *client);
static void evdev_repeat_callout(void *arg);
static void evdev_start_repeat(evdev_dev_t *evdev, uint16_t key);
static void evdev_stop_repeat(evdev_dev_t *evdev);

/* Device related functions */

static int evdev_read(file_t *f, uio_t *uio);
static int evdev_open(vnode_t *v, int mode, file_t *fp);
static int evdev_close(vnode_t *v, file_t *fp);
static int evdev_ioctl(file_t *f, u_long cmd, void *data);

static fileops_t evdev_fileops = {
  .fo_read = evdev_read,
  .fo_close = default_vnclose,
  .fo_stat = default_vnstat,
  .fo_ioctl = evdev_ioctl,
};

static vnodeops_t evdev_vnodeops = {
  .v_open = evdev_open,
  .v_close = evdev_close,
};

static void evdev_client_destroy(evdev_client_t *client) {
  WITH_MTX_LOCK ((&client->ec_evdev->ev_lock)) {
    evdev_dispose_client(client->ec_evdev, client);
  }

  mtx_destroy(&client->ec_buffer_mtx);
  cv_destroy(&client->ec_buffer_cv);
  kfree(M_DEV, client);
}

static int evdev_client_wait(evdev_client_t *client) {
  assert(mtx_owned(&client->ec_buffer_mtx));

  int error = cv_wait_intr(&client->ec_buffer_cv, &client->ec_buffer_mtx);
  if (error == EINTR)
    return ERESTARTSYS;
  return error;
}

static timeval_t evdev_client_gettime(evdev_client_t *client) {
  timeval_t now;
  bintime_t btnow;

  switch (client->ec_clock_id) {
    case EV_CLOCK_BOOTTIME:
      /* FALLTHROUGH */
    case EV_CLOCK_MONOTONIC:
      btnow = binuptime();
      break;
    case EV_CLOCK_REALTIME:
    default:
      btnow = bintime();
      break;
  }

  bt2tv(&btnow, &now);
  return now;
}

static void evdev_client_push(evdev_client_t *client, uint16_t type,
                              uint16_t code, int32_t value) {
  assert(mtx_owned(&client->ec_buffer_mtx));

  timeval_t time;
  size_t count, head, tail, ready;

  head = client->ec_buffer_head;
  tail = client->ec_buffer_tail;
  ready = client->ec_buffer_ready;
  count = client->ec_buffer_size;

  /* If queue is full drop its content and place SYN_DROPPED event */
  if ((tail + 1) % count == head) {
    head = (tail + count - 1) % count;
    client->ec_buffer[head] =
      (struct input_event){.type = EV_SYN, .code = SYN_DROPPED, .value = 0};
    client->ec_buffer_head = head;
    client->ec_buffer_ready = head;
  }

  client->ec_buffer[tail].type = type;
  client->ec_buffer[tail].code = code;
  client->ec_buffer[tail].value = value;
  client->ec_buffer_tail = (tail + 1) % count;

  /* Allow users to read events only after report has been completed */
  if (type == EV_SYN && code == SYN_REPORT) {
    time = evdev_client_gettime(client);
    for (; ready != client->ec_buffer_tail; ready = (ready + 1) % count)
      client->ec_buffer[ready].time = time;
    client->ec_buffer_ready = client->ec_buffer_tail;
  }
}

static void evdev_notify_event(evdev_client_t *client) {
  assert(mtx_owned((&client->ec_buffer_mtx)));
  cv_broadcast(&client->ec_buffer_cv);
}

static int evdev_read(file_t *f, uio_t *uio) {
  struct input_event event;
  struct input_event *head;
  int remaining, ret = 0;
  size_t evsize = sizeof(struct input_event);
  evdev_client_t *client = f->f_data;

  uio->uio_offset = 0; /* This device does not support offsets. */

  /* Zero-sized reads are allowed for error checking */
  if (uio->uio_resid != 0 && uio->uio_resid < evsize)
    return EINVAL;

  remaining = uio->uio_resid / evsize;

  mtx_lock(&client->ec_buffer_mtx);

  if (evdev_client_emptyq(client) && remaining != 0)
    ret = evdev_client_wait(client);

  while (ret == 0 && !evdev_client_emptyq(client) && remaining > 0) {
    head = client->ec_buffer + client->ec_buffer_head;
    bcopy(head, &event, evsize);
    client->ec_buffer_head =
      (client->ec_buffer_head + 1) % client->ec_buffer_size;
    remaining--;

    mtx_unlock(&client->ec_buffer_mtx);
    ret = uiomove(&event, evsize, uio);
    mtx_lock(&client->ec_buffer_mtx);
  }

  mtx_unlock(&client->ec_buffer_mtx);

  return ret;
}

static int evdev_open(vnode_t *v, int mode, file_t *fp) {
  int ret;
  evdev_client_t *client;
  evdev_dev_t *evdev = devfs_node_data(v);

  if ((ret = vnode_open_generic(v, mode, fp)))
    return ret;

  size_t buffer_size = 32;
  client = kmalloc(
    M_DEV, sizeof(evdev_client_t) + sizeof(struct input_event) * buffer_size,
    M_WAITOK | M_ZERO);

  client->ec_buffer_size = buffer_size;

  client->ec_evdev = evdev;
  mtx_init(&client->ec_buffer_mtx, 0);
  cv_init(&client->ec_buffer_cv, "ec_buffer_cv");

  WITH_MTX_LOCK (&(evdev->ev_lock)) {
    /* Avoid race with evdev_unregister */
    if (devfs_node_data(v) == NULL)
      ret = ENODEV;

    evdev_register_client(evdev, client);
  }

  if (ret != 0)
    kfree(M_DEV, client);
  else {
    fp->f_ops = &evdev_fileops;
    fp->f_data = client;
  }

  return ret;
}

static int evdev_close(vnode_t *v, file_t *fp) {
  evdev_client_t *client = fp->f_data;
  evdev_client_destroy(client);
  return 0;
}

static int evdev_ioctl_eviocgbit(evdev_dev_t *evdev, int type, int len,
                                 caddr_t data) {
  bitstr_t *bitmap;
  int limit;

  switch (type) {
    case 0:
      bitmap = evdev->ev_type_flags;
      limit = EV_CNT;
      break;
    case EV_KEY:
      bitmap = evdev->ev_key_flags;
      limit = KEY_CNT;
      break;
    default:
      return EINVAL;
  }

  /*
   * Clear ioctl data buffer in case it's bigger than
   * bitmap size
   */
  bzero(data, len);

  limit = bitstr_size(limit);
  len = min(limit, len);
  memcpy(data, bitmap, len);
  // td->td_retval[0] = len; TODO
  return 0;
}

static int evdev_ioctl(file_t *f, u_long cmd, void *data) {
  int limit, type_num;
  size_t len;
  evdev_client_t *client = f->f_data;
  evdev_dev_t *evdev = client->ec_evdev;

  len = IOCPARM_LEN(cmd);

  /* evdev fixed-length ioctls handling */
  switch (cmd) {
    case EVIOCGVERSION:
      *(int *)data = EV_VERSION;
      return 0;
    case EVIOCGID:
      memcpy(data, &evdev->ev_id, sizeof(struct input_id));
      return 0;
    case EVIOCGREP:
      if (!evdev_event_supported(evdev, EV_REP))
        return ENOTSUP;

      WITH_MTX_LOCK (&evdev->ev_lock) {
        memcpy(data, evdev->ev_rep, sizeof(evdev->ev_rep));
      }
      return 0;
    case EVIOCSCLOCKID:
      switch (*(int *)data) {
        case CLOCK_REALTIME:
          client->ec_clock_id = EV_CLOCK_REALTIME;
          return 0;
        case CLOCK_MONOTONIC:
          client->ec_clock_id = EV_CLOCK_MONOTONIC;
          return 0;
        default:
          return EINVAL;
      }
  }

  /* evdev variable-length ioctls handling */
  switch (IOCBASECMD(cmd)) {
    case EVIOCGNAME(0):
      /* Linux evdev does not terminate truncated strings with 0 */
      limit = min(strlen(evdev->ev_name) + 1, len);
      memcpy(data, evdev->ev_name, limit);
      // td->td_retval[0] = limit; TODO
      return 0;

    case EVIOCGKEY(0):
      limit = min(len, bitstr_size(KEY_CNT));
      WITH_MTX_LOCK (&evdev->ev_lock) {
        memcpy(data, evdev->ev_key_states, limit);
      }
      // td->td_retval[0] = limit; TODO
      return 0;

    case EVIOCGBIT(0, 0)... EVIOCGBIT(EV_MAX, 0):
      type_num = IOCBASECMD(cmd) - EVIOCGBIT(0, 0);
      return evdev_ioctl_eviocgbit(evdev, type_num, len, data);
  }

  return EINVAL;
}

static int evdev_dev_create(evdev_dev_t *evdev) {
  char buf[16];
  int ret, unit = 0;

  do {
    snprintf(buf, sizeof(buf), "event%d", unit);
    ret = devfs_makedev(evdev_input_dir, buf, &evdev_vnodeops, evdev, NULL);
  } while (ret == EEXIST);

  if (ret == 0)
    evdev->ev_unit = unit;

  return ret;
}

static bool evdev_parse_event(evdev_dev_t *evdev, uint16_t type, uint16_t code,
                              int32_t value) {
  assert(mtx_owned(&evdev->ev_lock));

  switch (type) {
    case EV_KEY:
      switch (value) {
        case KEY_EVENT_UP:
        case KEY_EVENT_DOWN:
          if (bit_test(evdev->ev_key_states, code) == (uint32_t)value)
            return true;

          bit_change(evdev->ev_key_states, code, value);
          break;

        case KEY_EVENT_REPEAT:
          if (bit_test(evdev->ev_key_states, code) == 0 ||
              !evdev_event_supported(evdev, EV_REP))
            return true;
          break;

        default:
          return true;
      }
      break;
    case EV_REP:
      if (evdev->ev_rep[code] == value)
        return true;
      evdev_set_repeat_params(evdev, code, value);
      break;
  }

  return false;
}

static void evdev_propagate_event(evdev_dev_t *evdev, uint16_t type,
                                  uint16_t code, int32_t value) {
  assert(mtx_owned(&evdev->ev_lock));

  evdev_client_t *client;

  LIST_FOREACH (client, &evdev->ev_clients, ec_link) {
    WITH_MTX_LOCK (&client->ec_buffer_mtx) {
      evdev_client_push(client, type, code, value);
      if (type == EV_SYN && code == SYN_REPORT)
        evdev_notify_event(client);
    }
  }
}

static void evdev_send_event(evdev_dev_t *evdev, uint16_t type, uint16_t code,
                             int32_t value) {
  bool skip_event = evdev_parse_event(evdev, type, code, value);
  if (!skip_event) {
    evdev_propagate_event(evdev, type, code, value);
  }
}

static void evdev_register_client(evdev_dev_t *evdev, evdev_client_t *client) {
  assert(mtx_owned(&evdev->ev_lock));
  LIST_INSERT_HEAD(&evdev->ev_clients, client, ec_link);
}

static void evdev_dispose_client(evdev_dev_t *evdev, evdev_client_t *client) {
  assert(mtx_owned(&evdev->ev_lock));

  LIST_REMOVE(client, ec_link);
  if (LIST_EMPTY(&evdev->ev_clients)) {
    if (evdev_event_supported(evdev, EV_REP) &&
        bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
      evdev_stop_repeat(evdev);
    }
  }
}

static void evdev_repeat_callout(void *arg) {
  evdev_dev_t *evdev = (evdev_dev_t *)arg;

  evdev_send_event(evdev, EV_KEY, evdev->ev_rep_key, KEY_EVENT_REPEAT);
  evdev_send_event(evdev, EV_SYN, SYN_REPORT, 1);

  if (evdev->ev_rep[REP_PERIOD])
    callout_reschedule(&evdev->ev_rep_callout,
                       evdev->ev_rep[REP_PERIOD] * CLK_TCK / 1000);
  else
    evdev->ev_rep_key = KEY_RESERVED;
}

static void evdev_start_repeat(evdev_dev_t *evdev, uint16_t key) {
  assert(mtx_owned(&evdev->ev_lock));
  if (evdev->ev_rep[REP_DELAY]) {
    evdev->ev_rep_key = key;
    callout_schedule(&evdev->ev_rep_callout,
                     evdev->ev_rep[REP_DELAY] * CLK_TCK / 1000);
  }
}

static void evdev_stop_repeat(evdev_dev_t *evdev) {
  assert(mtx_owned(&evdev->ev_lock));
  if (evdev->ev_rep_key != KEY_RESERVED) {
    callout_stop(&evdev->ev_rep_callout);
    evdev->ev_rep_key = KEY_RESERVED;
  }
}

/* Input device interface: */

evdev_dev_t *evdev_alloc(void) {
  return kmalloc(M_DEV, sizeof(evdev_dev_t), M_WAITOK | M_ZERO);
}

void evdev_free(evdev_dev_t *evdev) {
  kfree(M_DEV, evdev);
}

void evdev_set_name(evdev_dev_t *evdev, const char *name) {
  snprintf(evdev->ev_name, NAMELEN, "%s", name);
}

void evdev_set_id(evdev_dev_t *evdev, uint16_t bustype, uint16_t vendor,
                  uint16_t product, uint16_t version) {
  evdev->ev_id = (struct input_id){.bustype = bustype,
                                   .vendor = vendor,
                                   .product = product,
                                   .version = version};
}

void evdev_support_event(evdev_dev_t *evdev, uint16_t type) {
  assert(type < EV_CNT);
  bit_set(evdev->ev_type_flags, type);
}

void evdev_support_key(evdev_dev_t *evdev, uint16_t code) {
  assert(code < KEY_CNT);
  bit_set(evdev->ev_key_flags, code);
}

int evdev_register(evdev_dev_t *evdev) {
  mtx_init(&evdev->ev_lock, 0);
  LIST_INIT(&evdev->ev_clients);

  /* Initialize callout */
  callout_setup(&evdev->ev_rep_callout, evdev_repeat_callout, evdev);
  if (evdev->ev_rep[REP_DELAY] == 0 && evdev->ev_rep[REP_PERIOD] == 0) {
    /* Supply default values */
    evdev_set_repeat_params(evdev, REP_DELAY, 250);
    evdev_set_repeat_params(evdev, REP_PERIOD, 33);
  }

  return evdev_dev_create(evdev);
}

int evdev_unregister(evdev_dev_t *evdev) {
  return 0;
}

int evdev_push_event(evdev_dev_t *evdev, uint16_t type, uint16_t code,
                     int32_t value) {
  SCOPED_MTX_LOCK(&evdev->ev_lock);

  switch (type) {
    case EV_KEY:
      if (!evdev_event_supported(evdev, EV_REP))
        break;

      if (!bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
        /* Detect driver key repeats. */
        if (bit_test(evdev->ev_key_states, code) && value == KEY_EVENT_DOWN)
          value = KEY_EVENT_REPEAT;
      } else {
        /* Start/stop callout for evdev repeats */
        if (bit_test(evdev->ev_key_states, code) == !value &&
            !LIST_EMPTY(&evdev->ev_clients)) {
          if (value == KEY_EVENT_DOWN)
            evdev_start_repeat(evdev, code);
          else
            evdev_stop_repeat(evdev);
        }
      }
      break;
  }

  evdev_send_event(evdev, type, code, value);

  return 0;
}

/* Utility functions */

#define NONE KEY_RESERVED

/* clang-format off */
static uint16_t evdev_usb_scancodes[256] = {
	/* 0x00 - 0x27 */
	NONE,	NONE,	NONE,	NONE,	KEY_A,	KEY_B,	KEY_C,	KEY_D,
	KEY_E,	KEY_F,	KEY_G,	KEY_H,	KEY_I,	KEY_J,	KEY_K,	KEY_L,
	KEY_M,	KEY_N,	KEY_O,	KEY_P,	KEY_Q,	KEY_R,	KEY_S,	KEY_T,
	KEY_U,	KEY_V,	KEY_W,	KEY_X,	KEY_Y,	KEY_Z,	KEY_1,	KEY_2,
	KEY_3,	KEY_4,	KEY_5,	KEY_6,	KEY_7,	KEY_8,	KEY_9,	KEY_0,
	/* 0x28 - 0x3f */
	KEY_ENTER,	KEY_ESC,	KEY_BACKSPACE,	KEY_TAB,
	KEY_SPACE,	KEY_MINUS,	KEY_EQUAL,	KEY_LEFTBRACE,
	KEY_RIGHTBRACE,	KEY_BACKSLASH,	KEY_BACKSLASH,	KEY_SEMICOLON,
	KEY_APOSTROPHE,	KEY_GRAVE,	KEY_COMMA,	KEY_DOT,
	KEY_SLASH,	KEY_CAPSLOCK,	KEY_F1,		KEY_F2,
	KEY_F3,		KEY_F4,		KEY_F5,		KEY_F6,
	/* 0x40 - 0x5f */
	KEY_F7,		KEY_F8,		KEY_F9,		KEY_F10,
	KEY_F11,	KEY_F12,	KEY_SYSRQ,	KEY_SCROLLLOCK,
	KEY_PAUSE,	KEY_INSERT,	KEY_HOME,	KEY_PAGEUP,
	KEY_DELETE,	KEY_END,	KEY_PAGEDOWN,	KEY_RIGHT,
	KEY_LEFT,	KEY_DOWN,	KEY_UP,		KEY_NUMLOCK,
	KEY_KPSLASH,	KEY_KPASTERISK,	KEY_KPMINUS,	KEY_KPPLUS,
	KEY_KPENTER,	KEY_KP1,	KEY_KP2,	KEY_KP3,
	KEY_KP4,	KEY_KP5,	KEY_KP6,	KEY_KP7,
	/* 0x60 - 0x7f */
	KEY_KP8,	KEY_KP9,	KEY_KP0,	KEY_KPDOT,
	KEY_102ND,	KEY_COMPOSE,	KEY_POWER,	KEY_KPEQUAL,
	KEY_F13,	KEY_F14,	KEY_F15,	KEY_F16,
	KEY_F17,	KEY_F18,	KEY_F19,	KEY_F20,
	KEY_F21,	KEY_F22,	KEY_F23,	KEY_F24,
	KEY_OPEN,	KEY_HELP,	KEY_PROPS,	KEY_FRONT,
	KEY_STOP,	KEY_AGAIN,	KEY_UNDO,	KEY_CUT,
	KEY_COPY,	KEY_PASTE,	KEY_FIND,	KEY_MUTE,
	/* 0x80 - 0x9f */
	KEY_VOLUMEUP,	KEY_VOLUMEDOWN,	NONE,		NONE,
	NONE,		KEY_KPCOMMA,	NONE,		KEY_RO,
	KEY_KATAKANAHIRAGANA,	KEY_YEN,KEY_HENKAN,	KEY_MUHENKAN,
	KEY_KPJPCOMMA,	NONE,		NONE,		NONE,
	KEY_HANGEUL,	KEY_HANJA,	KEY_KATAKANA,	KEY_HIRAGANA,
	KEY_ZENKAKUHANKAKU,	NONE,	NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	/* 0xa0 - 0xbf */
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	/* 0xc0 - 0xdf */
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	/* 0xe0 - 0xff */
	KEY_LEFTCTRL,	KEY_LEFTSHIFT,	KEY_LEFTALT,	KEY_LEFTMETA,
	KEY_RIGHTCTRL,	KEY_RIGHTSHIFT,	KEY_RIGHTALT,	KEY_RIGHTMETA,
	KEY_PLAYPAUSE,	KEY_STOPCD,	KEY_PREVIOUSSONG,KEY_NEXTSONG,
	KEY_EJECTCD,	KEY_VOLUMEUP,	KEY_VOLUMEDOWN,	KEY_MUTE,
	KEY_WWW,	KEY_BACK,	KEY_FORWARD,	KEY_STOP,
	KEY_FIND,	KEY_SCROLLUP,	KEY_SCROLLDOWN,	KEY_EDIT,
	KEY_SLEEP,	KEY_COFFEE,	KEY_REFRESH,	KEY_CALC,
	NONE,		NONE,		NONE,		NONE,
};

static uint16_t evdev_at_set1_scancodes[] = {
	/* 0x00 - 0x1f */
	NONE,		KEY_ESC,	KEY_1,		KEY_2,
	KEY_3,		KEY_4,		KEY_5,		KEY_6,
	KEY_7,		KEY_8,		KEY_9,		KEY_0,
	KEY_MINUS,	KEY_EQUAL,	KEY_BACKSPACE,	KEY_TAB,
	KEY_Q,		KEY_W,		KEY_E,		KEY_R,
	KEY_T,		KEY_Y,		KEY_U,		KEY_I,
	KEY_O,		KEY_P,		KEY_LEFTBRACE,	KEY_RIGHTBRACE,
	KEY_ENTER,	KEY_LEFTCTRL,	KEY_A,		KEY_S,
	/* 0x20 - 0x3f */
	KEY_D,		KEY_F,		KEY_G,		KEY_H,
	KEY_J,		KEY_K,		KEY_L,		KEY_SEMICOLON,
	KEY_APOSTROPHE,	KEY_GRAVE,	KEY_LEFTSHIFT,	KEY_BACKSLASH,
	KEY_Z,		KEY_X,		KEY_C,		KEY_V,
	KEY_B,		KEY_N,		KEY_M,		KEY_COMMA,
	KEY_DOT,	KEY_SLASH,	KEY_RIGHTSHIFT,	KEY_KPASTERISK,
	KEY_LEFTALT,	KEY_SPACE,	KEY_CAPSLOCK,	KEY_F1,
	KEY_F2,		KEY_F3,		KEY_F4,		KEY_F5,
	/* 0x40 - 0x5f */
	KEY_F6,		KEY_F7,		KEY_F8,		KEY_F9,
	KEY_F10,	KEY_NUMLOCK,	KEY_SCROLLLOCK,	KEY_KP7,
	KEY_KP8,	KEY_KP9,	KEY_KPMINUS,	KEY_KP4,
	KEY_KP5,	KEY_KP6,	KEY_KPPLUS,	KEY_KP1,
	KEY_KP2,	KEY_KP3,	KEY_KP0,	KEY_KPDOT,
	NONE,		NONE,		KEY_102ND,	KEY_F11,
	KEY_F12,	NONE,		NONE,		NONE,
	NONE,		KEY_F13,	NONE,		NONE,
	/* 0x60 - 0x7f */
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	KEY_KATAKANAHIRAGANA,	KEY_HANGEUL,	KEY_HANJA,	KEY_RO,
	NONE,		NONE,	KEY_ZENKAKUHANKAKU,	KEY_HIRAGANA,
	KEY_KATAKANA,	KEY_HENKAN,	NONE,		KEY_MUHENKAN,
	NONE,		KEY_YEN,	KEY_KPCOMMA,	NONE,
	/* 0x00 - 0x1f. 0xE0 prefixed */
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	KEY_PREVIOUSSONG,	NONE,	NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		KEY_NEXTSONG,	NONE,		NONE,
	KEY_KPENTER,	KEY_RIGHTCTRL,	NONE,		NONE,
	/* 0x20 - 0x3f. 0xE0 prefixed */
	KEY_MUTE,	KEY_CALC,	KEY_PLAYPAUSE,	NONE,
	KEY_STOPCD,	NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		KEY_VOLUMEDOWN,	NONE,
	KEY_VOLUMEUP,	NONE,		KEY_HOMEPAGE,	NONE,
	NONE,		KEY_KPSLASH,	NONE,		KEY_SYSRQ,
	KEY_RIGHTALT,	NONE,		NONE,		KEY_F13,
	KEY_F14,	KEY_F15,	KEY_F16,	KEY_F17,
	/* 0x40 - 0x5f. 0xE0 prefixed */
	KEY_F18,	KEY_F19,	KEY_F20,	KEY_F21,
	KEY_F22,	NONE,		KEY_PAUSE,	KEY_HOME,
	KEY_UP,		KEY_PAGEUP,	NONE,		KEY_LEFT,
	NONE,		KEY_RIGHT,	NONE,		KEY_END,
	KEY_DOWN,	KEY_PAGEDOWN,	KEY_INSERT,	KEY_DELETE,
	NONE,		NONE,		NONE,		KEY_F23,
	KEY_F24,	NONE,		NONE,		KEY_LEFTMETA,
	KEY_RIGHTMETA,	KEY_MENU,	KEY_POWER,	KEY_SLEEP,
	/* 0x60 - 0x7f. 0xE0 prefixed */
	NONE,		NONE,		NONE,		KEY_WAKEUP,
	NONE,		KEY_SEARCH,	KEY_BOOKMARKS,	KEY_REFRESH,
	KEY_STOP,	KEY_FORWARD,	KEY_BACK,	KEY_COMPUTER,
	KEY_MAIL,	KEY_MEDIA,	NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
	NONE,		NONE,		NONE,		NONE,
};
/* clang-format on */

uint16_t evdev_hid2key(int scancode) {
  return evdev_usb_scancodes[scancode];
}

uint16_t evdev_scancode2key(int *state, int scancode) {
  uint16_t keycode;

  /* translate the scan code into a keycode */
  keycode = evdev_at_set1_scancodes[scancode & 0x7f];
  switch (*state) {
    case 0x00: /* normal scancode */
      switch (scancode) {
        case 0xE0:
        case 0xE1:
          *state = scancode;
          return NONE;
      }
      break;
    case 0xE0: /* 0xE0 prefix */
      *state = 0;
      keycode = evdev_at_set1_scancodes[0x80 + (scancode & 0x7f)];
      break;
    case 0xE1: /* 0xE1 prefix */
      /*
       * The pause/break key on the 101 keyboard produces:
       * E1-1D-45 E1-9D-C5
       * Ctrl-pause/break produces:
       * E0-46 E0-C6 (See above.)
       */
      *state = 0;
      if ((scancode & 0x7f) == 0x1D)
        *state = scancode;
      return NONE;
      /* NOT REACHED */
    case 0x1D: /* pause / break */
    case 0x9D:
      if ((*state ^ scancode) & 0x80)
        return NONE;
      *state = 0;
      if ((scancode & 0x7f) != 0x45)
        return NONE;
      keycode = KEY_PAUSE;
      break;
  }

  return keycode;
}

void evdev_support_all_known_keys(struct evdev_dev *evdev) {
  size_t nitems = sizeof(evdev_at_set1_scancodes) / sizeof(uint16_t);
  for (size_t i = KEY_RESERVED; i < nitems; i++)
    if (evdev_at_set1_scancodes[i] != NONE)
      evdev_support_key(evdev, evdev_at_set1_scancodes[i]);
}

static void init_evdev(void) {
  if (devfs_makedir(NULL, "input", &evdev_input_dir) != 0)
    panic("failed to create /dev/input directory");
}

SET_ENTRY(devfs_init, init_evdev);
