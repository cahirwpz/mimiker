/*
 * Heavily based on FreeBSD's code in `sys/dev/evdev/`.
 */

#include <dev/evdev.h>
#include <sys/devfs.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/devfs.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <bitstring.h>
#include <sys/libkern.h>

/* Maximum length of an evdev device */
#define EVDEV_NAMELEN 80

/* Default parameters for a software emulation of key repetition. */
#define DEFAULT_REP_DELAY 250
#define DEFAULT_REP_PERIOD 33

/* The size of client's event queue (counted in the number of events) */
#define CLIENT_QUEUE_SIZE 32

static inline void bit_change(bitstr_t *bitmap, int bit, int value) {
  if (value)
    bit_set(bitmap, bit);
  else
    bit_clear(bitmap, bit);
}

/* evdev clock IDs in Linux semantic */
typedef enum {
  EV_CLOCK_REALTIME = 0, /* UTC clock */
  EV_CLOCK_MONOTONIC,    /* monotonic, stops on suspend */
  EV_CLOCK_BOOTTIME      /* monotonic, suspend-awared */
} evdev_clock_id_t;

/* The vnode of the /dev/input directory. */
static devfs_node_t *evdev_input_dir;

/*
 * `evdev_dev_t` is the main structure of every input device. It contains both
 * general informations about the device and its state. A name, device ID and
 * supported features flags are only used by user-space.
 *
 * `evdev_client_t` is created whenever the evdev file is opened. Its jobs is to
 * handle an interaction with the user-space. Every client has its private event
 * buffer. Every time an input event is issued, it firstly goes through
 * `evdev_dev_t` and then it is propagated to clients. This allows us to have
 * multiple, non-interfering readers of a single evdev device. Hence, every
 * input device has exactly one `evdev_dev_t`, but possibly can have many
 * `evdev_client_t` structures linked to it.
 *
 * Field markings and the corresponding locks:
 * (!) - read-only access
 * (s) - state lock. The data it protects is changed
 *			      by incoming evdev events and some ioctls.
 * (c) - client locks. One lock per client to serialize access to data
 *			      available through character device node.
 */

typedef struct evdev_dev {
  char ev_name[EVDEV_NAMELEN]; /* (!) name of the device */
  struct input_id ev_id;       /* (!) device ID */

  /* Supported features: */
  bitstr_t bit_decl(ev_type_flags, EV_CNT);    /* (!) supported event types */
  bitstr_t bit_decl(ev_key_flags, KEY_CNT);    /* (!) supported key codes */
  bitstr_t bit_decl(ev_flags, EVDEV_FLAG_CNT); /* (!) supported features */

  mtx_t ev_lock; /* state lock */

  /* Repeat parameters & callout: */
  int ev_rep[REP_CNT]; /* (s) key repetition parameters (period and delay) */
  callout_t ev_rep_callout; /* (s) callout used for scheduling repetition */
  uint16_t ev_rep_key;      /* (s) currently repeated key */

  /* State: */
  bitstr_t bit_decl(ev_key_states, KEY_CNT); /* (s) state of all keys */

  LIST_HEAD(, evdev_client)
  ev_clients; /* (s) list of clients associated with this device */
} evdev_dev_t;

/*
 * Events in an evdev client are stored in ringbuffer. The important thing is
 * that we don't allow user-space to read events until the EV_SYN is
 * received. So besides keeping the tail of the ring buffer which denotes a
 * position of the very last element, we also keep track of an additional place,
 * here called, `ec_buffer_ready`, which points to the last event available for
 * a user.
 */
typedef struct evdev_client {
  evdev_dev_t *ec_evdev; /* (!) associated evdev device */
  evdev_clock_id_t
    ec_clock_id; /* (c) clock used for assigning time to events */

  LIST_ENTRY(evdev_client) ec_link; /* (s) link on client list */

  mtx_t ec_mtx;              /* client lock */
  condvar_t ec_buffer_cv;    /* (c) */
  size_t ec_buffer_size;     /* (c) */
  size_t ec_buffer_ready;    /* (c) last available event to read (read the note
                                above for details) */
  size_t ec_buffer_head;     /* (c) */
  size_t ec_buffer_tail;     /* (c) */
  input_event_t ec_buffer[]; /* (c) */
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

/* Unlink and free a `evdev_client_t` structure. */
static void evdev_client_destroy(evdev_client_t *client) {
  WITH_MTX_LOCK (&client->ec_evdev->ev_lock) {
    evdev_dispose_client(client->ec_evdev, client);
  }

  mtx_destroy(&client->ec_mtx);
  cv_destroy(&client->ec_buffer_cv);
  kfree(M_DEV, client);
}

/* Wait for the events */
static int evdev_client_wait(evdev_client_t *client) {
  assert(mtx_owned(&client->ec_mtx));

  int error = cv_wait_intr(&client->ec_buffer_cv, &client->ec_mtx);
  if (error == EINTR)
    return ERESTARTSYS;
  return error;
}

/* Get time depending on the client clock id */
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

/* Push a single event to client's queue */
static void evdev_client_push(evdev_client_t *client, uint16_t type,
                              uint16_t code, int32_t value) {
  assert(mtx_owned(&client->ec_mtx));

  timeval_t time;
  size_t count, head, tail, ready;

  head = client->ec_buffer_head;
  tail = client->ec_buffer_tail;
  ready = client->ec_buffer_ready;
  count = client->ec_buffer_size;

  /* If queue is full drop its content and place SYN_DROPPED event */
  if (tail + 1 == head || (tail + 1 - count) == head) {
    head = ready = 0;
    client->ec_buffer[head] =
      (input_event_t){.type = EV_SYN, .code = SYN_DROPPED, .value = 0};
    client->ec_buffer_head = head;
    client->ec_buffer_ready = head;
    tail = 1;
  }

  client->ec_buffer[tail].type = type;
  client->ec_buffer[tail].code = code;
  client->ec_buffer[tail].value = value;

  tail = (tail == count - 1) ? 0 : (tail + 1);
  client->ec_buffer_tail = tail;

  /* Allow users to read events only after report has been completed */
  if (type == EV_SYN && code == SYN_REPORT) {
    time = evdev_client_gettime(client);
    while (ready != client->ec_buffer_tail) {
      client->ec_buffer[ready].time = time;
      ready = (ready == count - 1) ? 0 : (ready + 1);
    }
    client->ec_buffer_ready = client->ec_buffer_tail;
  }
}

static void evdev_notify_event(evdev_client_t *client) {
  assert(mtx_owned((&client->ec_mtx)));
  cv_broadcast(&client->ec_buffer_cv);
}

/* Pop one event from the client's queue and return true if not empty. Otherwise
 * return false.
 */
static bool evdev_client_popq(evdev_client_t *client, input_event_t *ret) {
  assert(mtx_owned(&client->ec_mtx));

  input_event_t *head;

  if (evdev_client_emptyq(client))
    return false;

  head = client->ec_buffer + client->ec_buffer_head;
  bcopy(head, ret, sizeof(input_event_t));
  client->ec_buffer_head = client->ec_buffer_head + 1;
  if (client->ec_buffer_head == client->ec_buffer_size)
    client->ec_buffer_head = 0;

  return true;
}

static int evdev_read(file_t *f, uio_t *uio) {
  input_event_t event;
  int remaining, ret = 0;
  evdev_client_t *client = f->f_data;

  uio->uio_offset = 0; /* This device does not support offsets. */

  /* Zero-sized reads are allowed for error checking */
  if (uio->uio_resid != 0 && uio->uio_resid < sizeof(input_event_t))
    return EINVAL;

  remaining = uio->uio_resid / sizeof(input_event_t);

  mtx_lock(&client->ec_mtx);

  if (evdev_client_emptyq(client) && remaining != 0)
    ret = evdev_client_wait(client);

  while (ret == 0 && remaining > 0 && evdev_client_popq(client, &event)) {
    remaining--;

    mtx_unlock(&client->ec_mtx);
    ret = uiomove(&event, sizeof(input_event_t), uio);
    mtx_lock(&client->ec_mtx);
  }

  mtx_unlock(&client->ec_mtx);

  return ret;
}

static int evdev_open(vnode_t *v, int mode, file_t *fp) {
  int ret;
  evdev_client_t *client;
  evdev_dev_t *evdev = devfs_node_data(v);

  if ((ret = vnode_open_generic(v, mode, fp)))
    return ret;

  client = kmalloc(
    M_DEV, sizeof(evdev_client_t) + sizeof(input_event_t) * CLIENT_QUEUE_SIZE,
    M_WAITOK | M_ZERO);

  client->ec_buffer_size = CLIENT_QUEUE_SIZE;

  client->ec_evdev = evdev;
  mtx_init(&client->ec_mtx, 0);
  cv_init(&client->ec_buffer_cv, "ec_buffer_cv");

  WITH_MTX_LOCK (&evdev->ev_lock) { evdev_register_client(evdev, client); }

  fp->f_ops = &evdev_fileops;
  fp->f_data = client;

  return ret;
}

static int evdev_close(vnode_t *v, file_t *fp) {
  evdev_client_t *client = fp->f_data;
  evdev_client_destroy(client);
  return 0;
}

/* Handle EVIOCGBIT ioctl, which returns the bitmaps of supported features. */
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
      WITH_MTX_LOCK (&client->ec_mtx) {
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
  }

  /* evdev variable-length ioctls handling */
  switch (IOCBASECMD(cmd)) {
    case EVIOCGNAME(0):
      /* Linux evdev does not terminate truncated strings with 0 */
      limit = min(strlen(evdev->ev_name) + 1, len);
      memcpy(data, evdev->ev_name, limit);
      return 0;

    case EVIOCGKEY(0):
      limit = min(len, bitstr_size(KEY_CNT));
      WITH_MTX_LOCK (&evdev->ev_lock) {
        memcpy(data, evdev->ev_key_states, limit);
      }
      return 0;

    case EVIOCGBIT(0, 0)... EVIOCGBIT(EV_MAX, 0):
      type_num = IOCBASECMD(cmd) - EVIOCGBIT(0, 0);
      return evdev_ioctl_eviocgbit(evdev, type_num, len, data);
  }

  return EINVAL;
}

/* Create a devfs device for a given evdev. */
static int evdev_dev_create(evdev_dev_t *evdev) {
  char buf[16];
  int ret, unit = 0;

  /*
   * Find the first free number for that device. Not the best solution, but will
   * do for now
   */
  do {
    snprintf(buf, sizeof(buf), "event%d", unit);
    ret = devfs_makedev(evdev_input_dir, buf, &evdev_vnodeops, evdev, NULL);
    unit++;
  } while (ret == EEXIST);

  return ret;
}

static bool evdev_handle_event_key(evdev_dev_t *evdev, uint16_t type,
                                   uint16_t code, int32_t value) {
  switch (value) {
    case KEY_EVENT_UP:
    case KEY_EVENT_DOWN:
      if (bit_test(evdev->ev_key_states, code) == (uint32_t)value)
        return false;

      bit_change(evdev->ev_key_states, code, value);
      break;

    case KEY_EVENT_REPEAT:
      if (bit_test(evdev->ev_key_states, code) == 0 ||
          !evdev_event_supported(evdev, EV_REP))
        return false;
      break;
  }

  return true;
}

static bool evdev_handle_event_rep(evdev_dev_t *evdev, uint16_t type,
                                   uint16_t code, int32_t value) {
  if (evdev->ev_rep[code] == value)
    return false;
  evdev_set_repeat_params(evdev, code, value);
  return true;
}

/* Handle incoming event. Returns false if the event should be skipped. */
static bool evdev_handle_event(evdev_dev_t *evdev, uint16_t type, uint16_t code,
                               int32_t value) {
  assert(mtx_owned(&evdev->ev_lock));

  if (type == EV_KEY)
    return evdev_handle_event_key(evdev, type, code, value);
  else if (type == EV_REP)
    return evdev_handle_event_rep(evdev, type, code, value);

  return true;
}

/* Propagate one event to all clients and notify them if it is the end of the
 * event batch. */
static void evdev_propagate_event(evdev_dev_t *evdev, uint16_t type,
                                  uint16_t code, int32_t value) {
  assert(mtx_owned(&evdev->ev_lock));

  evdev_client_t *client;

  LIST_FOREACH (client, &evdev->ev_clients, ec_link) {
    WITH_MTX_LOCK (&client->ec_mtx) {
      evdev_client_push(client, type, code, value);
      if (type == EV_SYN && code == SYN_REPORT)
        evdev_notify_event(client);
    }
  }
}

/* Send an event to evdev. */
static void evdev_send_event(evdev_dev_t *evdev, uint16_t type, uint16_t code,
                             int32_t value) {
  if (evdev_handle_event(evdev, type, code, value))
    evdev_propagate_event(evdev, type, code, value);
}

static void evdev_register_client(evdev_dev_t *evdev, evdev_client_t *client) {
  assert(mtx_owned(&evdev->ev_lock));
  LIST_INSERT_HEAD(&evdev->ev_clients, client, ec_link);
}

static void evdev_dispose_client(evdev_dev_t *evdev, evdev_client_t *client) {
  assert(mtx_owned(&evdev->ev_lock));

  LIST_REMOVE(client, ec_link);
  if (!LIST_EMPTY(&evdev->ev_clients))
    return;

  /* If there are no active clients left, stop the callout. */
  if (evdev_event_supported(evdev, EV_REP) &&
      bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
    evdev_stop_repeat(evdev);
  }
}

/* Callout function called every key repetition. */
static void evdev_repeat_callout(void *arg) {
  evdev_dev_t *evdev = (evdev_dev_t *)arg;

  SCOPED_MTX_LOCK(&evdev->ev_lock);

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

/*
 * Start or stop the key repetition callout depending on the key and its state.
 */
static void evdev_soft_repeat(evdev_dev_t *evdev, uint16_t key, int32_t state) {
  assert(mtx_owned(&evdev->ev_lock));

  /* Don't start the callout when there are no clients */
  if (LIST_EMPTY(&evdev->ev_clients))
    return;

  if (bit_test(evdev->ev_key_states, key) == (unsigned)state)
    return;

  if (state == KEY_EVENT_DOWN)
    evdev_start_repeat(evdev, key);
  else
    evdev_stop_repeat(evdev);
}

/* Evdev public interface (described in the header file). */

evdev_dev_t *evdev_alloc(void) {
  return kmalloc(M_DEV, sizeof(evdev_dev_t), M_WAITOK | M_ZERO);
}

void evdev_free(evdev_dev_t *evdev) {
  kfree(M_DEV, evdev);
}

void evdev_set_name(evdev_dev_t *evdev, const char *name) {
  snprintf(evdev->ev_name, EVDEV_NAMELEN, "%s", name);
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

void evdev_set_flag(evdev_dev_t *evdev, uint16_t flag) {
  assert(flag < EVDEV_FLAG_CNT);
  bit_set(evdev->ev_flags, flag);
}

int evdev_register(evdev_dev_t *evdev) {
  mtx_init(&evdev->ev_lock, 0);
  LIST_INIT(&evdev->ev_clients);

  /* Initialize callout */
  callout_setup(&evdev->ev_rep_callout, evdev_repeat_callout, evdev);
  if (evdev->ev_rep[REP_DELAY] == 0 && evdev->ev_rep[REP_PERIOD] == 0) {
    /* Supply default values */
    evdev_set_repeat_params(evdev, REP_DELAY, DEFAULT_REP_DELAY);
    evdev_set_repeat_params(evdev, REP_PERIOD, DEFAULT_REP_PERIOD);
  }

  return evdev_dev_create(evdev);
}

void evdev_push_event(evdev_dev_t *evdev, uint16_t type, uint16_t code,
                      int32_t value) {
  SCOPED_MTX_LOCK(&evdev->ev_lock);

  if (type == EV_KEY && evdev_event_supported(evdev, EV_REP)) {
    if (bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
      /* Start/stop callout for evdev repeats */
      evdev_soft_repeat(evdev, code, value);
    } else {
      /* Detect driver key repeats. */
      if (bit_test(evdev->ev_key_states, code) && value == KEY_EVENT_DOWN)
        value = KEY_EVENT_REPEAT;
    }
  }

  evdev_send_event(evdev, type, code, value);
}

static void init_evdev(void) {
  if (devfs_makedir(NULL, "input", &evdev_input_dir) != 0)
    panic("failed to create /dev/input directory");
}

SET_ENTRY(devfs_init, init_evdev);
