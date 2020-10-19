#define KL_LOG KL_TTY
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/termios.h>
#include <sys/tty.h>
#define TTYDEFCHARS
#include <sys/ttydefaults.h>
#undef TTYDEFCHARS
#include <sys/malloc.h>
#include <sys/kmem_flags.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/ringbuf.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/mimiker.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/vm_map.h>
#include <sys/device.h>

/* START OF FreeBSD CODE */

/*
 * Table with character classes and parity. The 8th bit indicates parity,
 * the 7th bit indicates the character is an alphameric or underscore (for
 * ALTWERASE), and the low 6 bits indicate delay type.  If the low 6 bits
 * are 0 then the character needs no special processing on output; classes
 * other than 0 might be translated or (not currently) require delays.
 */
#define E 0x00 /* Even parity. */
#define O 0x80 /* Odd parity. */
#define PARITY(c) (char_type[c] & O)

#define ALPHA 0x40 /* Alpha or underscore. */
#define ISALPHA(c) (char_type[(c)&TTY_CHARMASK] & ALPHA)

#define CCLASSMASK 0x3f
#define CCLASS(c) (char_type[c] & CCLASSMASK)

typedef enum {
  ORDINARY = 0,
  CONTROL = 1,
  BACKSPACE = 2,
  NEWLINE = 3,
  TAB = 4,
  VTAB = 5,
  RETURN = 6,
} cclass_t;

#define BS BACKSPACE
#define CC CONTROL
#define CR RETURN
#define NA ORDINARY | ALPHA
#define NL NEWLINE
#define NO ORDINARY
#define TB TAB
#define VT VTAB

/* clang-format off */
unsigned char const char_type[] = {
	E|CC, O|CC, O|CC, E|CC, O|CC, E|CC, E|CC, O|CC,	/* nul - bel */
	O|BS, E|TB, E|NL, O|CC, E|VT, O|CR, O|CC, E|CC,	/* bs - si */
	O|CC, E|CC, E|CC, O|CC, E|CC, O|CC, O|CC, E|CC,	/* dle - etb */
	E|CC, O|CC, O|CC, E|CC, O|CC, E|CC, E|CC, O|CC,	/* can - us */
	O|NO, E|NO, E|NO, O|NO, E|NO, O|NO, O|NO, E|NO,	/* sp - ' */
	E|NO, O|NO, O|NO, E|NO, O|NO, E|NO, E|NO, O|NO,	/* ( - / */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA,	/* 0 - 7 */
	O|NA, E|NA, E|NO, O|NO, E|NO, O|NO, O|NO, E|NO,	/* 8 - ? */
	O|NO, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA,	/* @ - G */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA,	/* H - O */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA,	/* P - W */
	O|NA, E|NA, E|NA, O|NO, E|NO, O|NO, O|NO, O|NA,	/* X - _ */
	E|NO, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA,	/* ` - g */
	O|NA, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA,	/* h - o */
	O|NA, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA,	/* p - w */
	E|NA, O|NA, O|NA, E|NO, O|NO, E|NO, E|NO, O|CC,	/* x - del */
	/*
	 * Meta chars; should be settable per character set;
	 * for now, treat them all as normal characters.
	 */
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
};
/* clang-format on */
#undef BS
#undef CC
#undef CR
#undef NA
#undef NL
#undef NO
#undef TB
#undef VT

/* END OF FreeBSD CODE */

/* termios flags that can be changed using TIOCSETA{,W,F}. */
#define TTYSUP_IFLAG_CHANGE (INLCR | IGNCR | ICRNL | IMAXBEL)
#define TTYSUP_OFLAG_CHANGE (OPOST | ONLCR | OCRNL | ONOCR | ONLRET)
#define TTYSUP_LFLAG_CHANGE 0

static void tty_output(tty_t *tty, uint8_t c);

/* Initialize termios with sane defaults. */
static void tty_init_termios(struct termios *tio) {
  tio->c_cflag = TTYDEF_CFLAG;
  tio->c_iflag = TTYDEF_IFLAG;
  tio->c_lflag = TTYDEF_LFLAG;
  tio->c_oflag = TTYDEF_OFLAG;
  tio->c_ispeed = TTYDEF_SPEED;
  tio->c_ospeed = TTYDEF_SPEED;
  memcpy(&tio->c_cc, ttydefchars, sizeof ttydefchars);
}

tty_t *tty_alloc(void) {
  tty_t *tty = kmalloc(M_DEV, sizeof(tty_t), M_WAITOK);
  mtx_init(&tty->t_lock, 0);
  ringbuf_init(&tty->t_inq, kmalloc(M_DEV, TTY_QUEUE_SIZE, M_WAITOK),
               TTY_QUEUE_SIZE);
  cv_init(&tty->t_incv, "t_incv");
  ringbuf_init(&tty->t_outq, kmalloc(M_DEV, TTY_QUEUE_SIZE, M_WAITOK),
               TTY_QUEUE_SIZE);
  tty_init_termios(&tty->t_termios);
  tty->t_ops.t_notify_out = NULL;
  tty->t_data = NULL;
  tty->t_column = 0;
  return tty;
}

/* Notify the serial device driver that there are characters
 * in the output queue. */
static void tty_notify_out(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  if (tty->t_outq.count > 0) {
    tty->t_ops.t_notify_out(tty);
  }
}

/*
 * Put a character directly in the output queue.
 * TODO: what to do when the queue is full?
 */
static void tty_outq_putc(tty_t *tty, uint8_t c) {
  if (!(tty->t_lflag & FLUSHO) && !ringbuf_putb(&tty->t_outq, c)) {
    klog("tty_outq_putc: outq full, dropping character 0x%hhx");
  }
}

/* Wake up readers waiting for input. */
static void tty_wakeup(tty_t *tty) {
  cv_broadcast(&tty->t_incv);
}

void tty_input(tty_t *tty, uint8_t c) {
  /* TODO: Canonical mode, character processing. */
  int iflag = tty->t_iflag;

  if (c == '\r') {
    if (iflag & IGNCR)
      return;
    else if (iflag & ICRNL)
      c = '\n';
  } else if (c == '\n' && (iflag & INLCR)) {
    c = '\r';
  }

  if (tty->t_inq.count >= TTY_QUEUE_SIZE) {
    if (iflag & IMAXBEL) {
      tty_output(tty, CTRL('g'));
      tty_notify_out(tty);
    }
    return;
  }

  ringbuf_putb(&tty->t_inq, c);
  tty_wakeup(tty);
}

static int tty_read(vnode_t *v, uio_t *uio, int ioflags) {
  tty_t *tty = v->v_data;
  int error = 0;

  uio->uio_offset = 0; /* This device does not support offsets. */

  uint8_t c;
  WITH_MTX_LOCK (&tty->t_lock) {
    while (tty->t_inq.count == 0)
      cv_wait(&tty->t_incv, &tty->t_lock);

    /* In raw mode, read a single character.
     * In theory we should respect things such as VMIN and VTIME, but most
     * programs don't use them. */
    ringbuf_getb(&tty->t_inq, &c);
    error = uiomove(&c, 1, uio);
  }

  return error;
}

static void tty_discard_input(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  ringbuf_reset(&tty->t_inq);
}

/*
 * Do output processing on a character.
 * `cb` points to a character buffer, where the first element
 * is the character to process. Upon return, the buffer contains
 * the characters that should be transmitted. The return value
 * is the number of characters that should be transmitted.
 */
static int tty_process_out(tty_t *tty, int oflag, uint8_t cb[2]) {
  uint8_t c = cb[0];
  /* Newline translation: if ONLCR is set, translate newline into "\r\n". */
  if (c == '\n' && (oflag & ONLCR)) {
    cb[0] = '\r';
    cb[1] = '\n';
    return 2;
  }
  if (c == '\r') {
    /* If OCRNL is set, translate "\r" into "\n". */
    if (oflag & OCRNL) {
      cb[0] = '\n';
      return 1;
    }
    /* If ONOCR is set, don't transmit CRs when on column 0. */
    if ((oflag & ONOCR) && tty->t_column == 0)
      return 0;
  }
  return 1;
}

/*
 * Add a single character to the output queue.
 * Output processing (e.g. turning NL into CR-NL pairs) happens here.
 */
static void tty_output(tty_t *tty, uint8_t c) {
  assert(mtx_owned(&tty->t_lock));

  int oflag = tty->t_oflag;
  /* Check if output processing is enabled. */
  if (!(oflag & OPOST)) {
    tty_outq_putc(tty, c);
    return;
  }

  uint8_t cb[2];
  cb[0] = c;
  int ccount = tty_process_out(tty, oflag, cb);
  int col = tty->t_column;

  for (int i = 0; i < ccount; i++) {
    tty_outq_putc(tty, cb[i]);
    switch (CCLASS(cb[i])) {
      case BACKSPACE:
        if (col > 0)
          col--;
        break;
      case CONTROL:
        break;
      case NEWLINE:
        if (oflag & ONLRET)
          col = 0;
        break;
      case RETURN:
        col = 0;
        break;
      case ORDINARY:
        col++;
        break;
      case TAB:
        /* XXX We assume that the terminal device has tabs of width 8.
         * This can be remediated by implementing the OXTABS option
         * which expands tabs into spaces. */
        col = roundup2(col + 1, 8);
        break;
    }
  }
  tty->t_column = col;
}

static int tty_write(vnode_t *v, uio_t *uio, int ioflags) {
  tty_t *tty = v->v_data;
  int error = 0;

  uio->uio_offset = 0; /* This device does not support offsets. */

  uint8_t c;
  WITH_MTX_LOCK (&tty->t_lock) {
    while (uio->uio_resid > 0) {
      error = uiomove(&c, 1, uio);
      if (error)
        break;
      tty_output(tty, c);
    }
    tty_notify_out(tty);
  }

  return error;
}

static int tty_close(vnode_t *v, file_t *fp) {
  /* TODO implement. */
  return 0;
}

static int tty_ioctl(vnode_t *v, u_long cmd, void *data) {
  tty_t *tty = v->v_data;
  int ret = 0;

  mtx_lock(&tty->t_lock);
  switch (cmd) {
    case TIOCGETA: { /* Get termios */
      struct termios *t = (struct termios *)data;
      memcpy(t, &tty->t_termios, sizeof(struct termios));
      break;
    }
    case TIOCSETA:    /* Set termios immediately */
    case TIOCSETAW:   /* Set termios after waiting for output to drain */
    case TIOCSETAF: { /* Like TIOCSETAW, but also discard all pending input */
      struct termios *t = (struct termios *)data;

      if (cmd == TIOCSETAW || cmd == TIOCSETAF) {
        /* TODO Drain all output. */
        if (cmd == TIOCSETAF) {
          /* Discard all pending input. */
          tty_discard_input(tty);
        }
      }

#define SET_MASKED_BITS(lhs, rhs, mask)                                        \
  (lhs) = ((lhs) & ~(mask)) | ((rhs) & (mask))
      SET_MASKED_BITS(tty->t_iflag, t->c_iflag, TTYSUP_IFLAG_CHANGE);
      SET_MASKED_BITS(tty->t_oflag, t->c_oflag, TTYSUP_OFLAG_CHANGE);
      SET_MASKED_BITS(tty->t_lflag, t->c_lflag, TTYSUP_LFLAG_CHANGE);
      /* Changing `t_cflag` is completely unsupported right now. */
#undef SET_MASKED_BITS
      memcpy(tty->t_cc, t->c_cc, sizeof(tty->t_cc));

      break;
    }
    case 0:
      ret = EPASSTHROUGH;
      break;
    default: {
      char buf[32];
      IOCSNPRINTF(buf, sizeof(buf), cmd);
      klog("Unsupported tty ioctl: %s", buf);
      unsigned len = IOCPARM_LEN(cmd);
      memset(data, 0, len);
      break;
    }
  }
  mtx_unlock(&tty->t_lock);

  return ret;
}

static int tty_getattr(vnode_t *v, vattr_t *va) {
  memset(va, 0, sizeof(vattr_t));
  va->va_mode = S_IFCHR;
  va->va_nlink = 1;
  va->va_ino = 0;
  va->va_size = 0;
  return 0;
}

vnodeops_t tty_vnodeops = {.v_open = vnode_open_generic,
                           .v_write = tty_write,
                           .v_read = tty_read,
                           .v_close = tty_close,
                           .v_ioctl = tty_ioctl,
                           .v_getattr = tty_getattr};
