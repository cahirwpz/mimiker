#define KL_LOG KL_TTY
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/termios.h>
#include <sys/tty.h>
#include <sys/ttydefaults.h>
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

#define ASCII_DEL '\x7f'
/* Control character should be echoed as ^X */
#define CTL_ECHO(c)                                                            \
  (((c) <= 0x1f && (c) != '\t' && (c) != '\n') || (c) == ASCII_DEL)

/* termios flags that can be changed using TIOCSETA{,W,F}. */
#define TTYSUP_IFLAG_CHANGE (INLCR | IGNCR | ICRNL | IMAXBEL)
#define TTYSUP_OFLAG_CHANGE (OPOST | ONLCR | OCRNL | ONOCR | ONLRET)
#define TTYSUP_LFLAG_CHANGE                                                    \
  (ECHOKE | ECHOE | ECHOK | ECHO | ECHONL | ECHOCTL | ICANON)

static void tty_output(tty_t *tty, uint8_t c);

static inline bool tty_is_break(tty_t *tty, uint8_t c) {
  return (c == '\n' || ((c == tty->t_cc[VEOF] || c == tty->t_cc[VEOL]) &&
                        c != _POSIX_VDISABLE));
}

/* Initialize termios with sane defaults. */
static void tty_init_termios(struct termios *tio) {

  /* Default control characters from FreeBSD */
  static const cc_t ttydefchars[NCCS] = {
    [VEOF] = CEOF,         [VEOL] = CEOL,
    [VEOL2] = CEOL,        [VERASE] = CERASE,
    [VWERASE] = CWERASE,   [VKILL] = CKILL,
    [VREPRINT] = CREPRINT, [7] = _POSIX_VDISABLE, /* spare */
    [VINTR] = CINTR,       [VQUIT] = CQUIT,
    [VSUSP] = CSUSP,       [VDSUSP] = CDSUSP,
    [VSTART] = CSTART,     [VSTOP] = CSTOP,
    [VLNEXT] = CLNEXT,     [VDISCARD] = CDISCARD,
    [VMIN] = CMIN,         [VTIME] = CTIME,
    [VSTATUS] = CSTATUS,   [19] = _POSIX_VDISABLE, /* spare */
  };

  tio->c_cflag = TTYDEF_CFLAG;
  tio->c_iflag = TTYDEF_IFLAG;
  tio->c_lflag = TTYDEF_LFLAG;
  tio->c_oflag = TTYDEF_OFLAG;
  tio->c_ispeed = TTYDEF_SPEED;
  tio->c_ospeed = TTYDEF_SPEED;
  memcpy(&tio->c_cc, ttydefchars, sizeof(ttydefchars));
}

tty_t *tty_alloc(void) {
  tty_t *tty = kmalloc(M_DEV, sizeof(tty_t), M_WAITOK);
  mtx_init(&tty->t_lock, 0);
  ringbuf_init(&tty->t_inq, kmalloc(M_DEV, TTY_QUEUE_SIZE, M_WAITOK),
               TTY_QUEUE_SIZE);
  cv_init(&tty->t_incv, "t_incv");
  ringbuf_init(&tty->t_outq, kmalloc(M_DEV, TTY_QUEUE_SIZE, M_WAITOK),
               TTY_QUEUE_SIZE);
  tty->t_line.ln_buf = kmalloc(M_TEMP, LINEBUF_SIZE, M_WAITOK);
  tty->t_line.ln_size = LINEBUF_SIZE;
  tty->t_line.ln_count = 0;
  tty_init_termios(&tty->t_termios);
  tty->t_ops.t_notify_out = NULL;
  tty->t_data = NULL;
  tty->t_column = 0;
  tty->t_rocol = tty->t_rocount = 0;
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

/* Line buffer helper functions */
static bool tty_line_putc(tty_t *tty, uint8_t c) {
  linebuf_t *line = &tty->t_line;
  if (line->ln_count >= line->ln_size) {
    return false;
  }
  line->ln_buf[line->ln_count++] = c;
  return true;
}

static bool tty_line_unputc(tty_t *tty, uint8_t *c) {
  linebuf_t *line = &tty->t_line;
  if (line->ln_count == 0) {
    return false;
  }
  *c = line->ln_buf[--line->ln_count];
  return true;
}

/*
 * Copy the line buffer to the input queue and reset the line buffer.
 * Returns true on success.
 * If there isn't enough space in the input queue, then some characters
 * from the line will be lost, and the function will return false.
 */
static bool tty_line_finish(tty_t *tty) {
  linebuf_t *line = &tty->t_line;
  uio_t uio = UIO_SINGLE_KERNEL(UIO_WRITE, 0, line->ln_buf, line->ln_count);
  ringbuf_write(&tty->t_inq, &uio);
  line->ln_count = 0;
  return (uio.uio_resid == 0);
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

/*
 * Echo a typed character to the terminal.
 * Call with `tty->t_lock` held.
 */
static void tty_echo(tty_t *tty, uint8_t c) {
  assert(mtx_owned(&tty->t_lock));

  int lflag = tty->t_lflag;
  /* If ECHO is not set, only echo newlines provided ECHONL is set. */
  if (!(lflag & ECHO) && (!(lflag & ECHONL) || c != '\n'))
    return;
  /* If ECHOCTL is set, echo control characters as ^A, ^B etc. */
  if ((lflag & ECHOCTL) && CTL_ECHO(c)) {
    tty_output(tty, '^');
    if (c == ASCII_DEL)
      c = '?';
    else
      c += 'A' - 1;
  }
  tty_output(tty, c);
}

static void tty_retype(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));

  /* Echo the reprint character. */
  if (tty->t_cc[VREPRINT] != _POSIX_VDISABLE)
    tty_echo(tty, tty->t_cc[VREPRINT]);

  tty_output(tty, '\n');

  linebuf_t *line = &tty->t_line;
  for (size_t i = 0; i < line->ln_count; i++) {
    tty_echo(tty, line->ln_buf[i]);
  }
  tty->t_rocount = line->ln_count;
  tty->t_rocol = 0;
}

static void tty_rubout(tty_t *tty, unsigned int count) {
  while (count--) {
    tty_output(tty, '\b');
    tty_output(tty, ' ');
    tty_output(tty, '\b');
  }
}

/* Erase a character from the line. */
static void tty_erase(tty_t *tty, uint8_t c) {
  assert(mtx_owned(&tty->t_lock));

  int lflag = tty->t_lflag;
  if (!(lflag & ECHO))
    return;
  if (lflag & ECHOE) {
    if (tty->t_rocount == 0) {
      /* Last char output didn't come from user: retype line.  */
      tty_retype(tty);
      return;
    }
    switch (CCLASS(c)) {
      case ORDINARY:
        tty_rubout(tty, 1);
        break;
      case BACKSPACE:
      case CONTROL:
      case NEWLINE:
      case RETURN:
      case VTAB:
        if (lflag & ECHOCTL) {
          /* Control characters are echoed as 2 characters (e.g. ^C) */
          tty_rubout(tty, 2);
        }
        break;
      case TAB:
        if (tty->t_rocount < tty->t_line.ln_count) {
          /* The line is interleaved with some process's output: retype line. */
          tty_retype(tty);
          return;
        }
        /*
         * In order to erase a TAB, we need to know how many spaces the TAB
         * takes up, which can be anywhere from 1 to 8. What follows is a mild
         * hack that computes the length of the line until just before the TAB
         * by re-echoing the whole line (which doesn't contain the TAB anymore),
         * but with output inhibited by using the FLUSHO flag. Once we compute
         * the length of the line, we can subtract it from the original column
         * number to get the width of the TAB.
         */
        size_t savecol = tty->t_column;
        tty->t_lflag |= FLUSHO;
        tty->t_column = tty->t_rocol;
        linebuf_t *line = &tty->t_line;
        for (size_t i = 0; i < line->ln_count; i++) {
          tty_echo(tty, line->ln_buf[i]);
        }
        tty->t_lflag &= ~FLUSHO;
        savecol -= tty->t_column;
        /* `savecol` now contains the width of the tab. */
        tty->t_column += savecol;
        if (savecol > 8)
          savecol = 8; /* Guard against overflow. */
        while (savecol-- > 0)
          tty_output(tty, '\b');
        break;
      default:
        klog("tty_erase: don't know how to erase character 0x%hhx (class %hh), "
             "ignoring.",
             c, CCLASS(c));
        break;
    }
  } else {
    tty_echo(tty, tty->t_cc[VERASE]);
  }
  tty->t_rocount--;
}

/* Wake up readers waiting for input. */
static void tty_wakeup(tty_t *tty) {
  cv_broadcast(&tty->t_incv);
}

static void tty_bell(tty_t *tty) {
  if (tty->t_iflag & IMAXBEL) {
    tty_output(tty, CTRL('g'));
    tty_notify_out(tty);
  }
}

void tty_input(tty_t *tty, uint8_t c) {
  /* TODO: Canonical mode, character processing. */
  int iflag = tty->t_iflag;
  int lflag = tty->t_lflag;
  uint8_t *cc = tty->t_cc;

  if (c == '\r') {
    if (iflag & IGNCR)
      return;
    else if (iflag & ICRNL)
      c = '\n';
  } else if (c == '\n' && (iflag & INLCR)) {
    c = '\r';
  }

  if (lflag & ICANON) {
    /* Canonical mode character processing takes place here. */
    if (CCEQ(cc[VERASE], c)) {
      /* Erase/backspace */
      uint8_t erased;
      if (tty_line_unputc(tty, &erased)) {
        tty_erase(tty, erased);
        tty_notify_out(tty);
      }
      return;
    }

    if (CCEQ(cc[VKILL], c)) {
      /* Kill: erase the whole line. */
      if ((lflag & ECHOKE) && tty->t_line.ln_count == tty->t_rocount) {
        uint8_t erased;
        while (tty_line_unputc(tty, &erased))
          tty_erase(tty, erased);
      } else {
        tty_echo(tty, c);
        if (lflag & (ECHOK | ECHOKE))
          tty_echo(tty, '\n');
        tty->t_line.ln_count = 0;
        tty->t_rocount = 0;
      }
      tty_notify_out(tty);
      return;
    }

    bool is_break = tty_is_break(tty, c);

    /* Check for possibility of overflow. */
    if ((tty->t_inq.count + tty->t_line.ln_count >= TTY_QUEUE_SIZE - 1 ||
         tty->t_line.ln_count == LINEBUF_SIZE - 1) &&
        !is_break) {
      tty_bell(tty);
      return;
    }

    tty_line_putc(tty, c);

    if (is_break) {
      tty_line_finish(tty);
      tty->t_rocount = 0;
      tty_wakeup(tty);
    } else if (tty->t_rocount++ == 0) {
      tty->t_rocol = tty->t_column;
    }
    size_t i = tty->t_column;
    tty_echo(tty, c);
    if (CCEQ(cc[VEOF], c) && (lflag & ECHO)) {
      /* Place the cursor over the '^' of the ^D. */
      i = MIN(2, tty->t_column - i);
      while (i--)
        tty_output(tty, '\b');
    }
    tty_notify_out(tty);
    return;
  } else {
    /* Raw (non-canonical) mode */
    if (tty->t_inq.count >= TTY_QUEUE_SIZE) {
      tty_bell(tty);
      return;
    }

    ringbuf_putb(&tty->t_inq, c);
    tty_wakeup(tty);
    return;
  }
}

static int tty_read(vnode_t *v, uio_t *uio, int ioflags) {
  tty_t *tty = v->v_data;
  int error = 0;

  uio->uio_offset = 0; /* This device does not support offsets. */

  uint8_t c;
  WITH_MTX_LOCK (&tty->t_lock) {
    while (tty->t_inq.count == 0)
      cv_wait(&tty->t_incv, &tty->t_lock);

    if (tty->t_lflag & ICANON) {
      /* In canonical mode, read as many characters as possible, but no more
       * than one line. */
      while (ringbuf_getb(&tty->t_inq, &c)) {
        if (CCEQ(tty->t_cc[VEOF], c))
          break;
        error = uiomove(&c, 1, uio);
        if (error)
          break;
        if (uio->uio_resid == 0)
          break;
        /* Check for end of line. */
        if (tty_is_break(tty, c))
          break;
      }
    } else {
      /* In raw mode, read a single character.
       * In theory we should respect things such as VMIN and VTIME, but most
       * programs don't use them. */
      ringbuf_getb(&tty->t_inq, &c);
      error = uiomove(&c, 1, uio);
    }
  }

  return error;
}

static void tty_discard_input(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  ringbuf_reset(&tty->t_inq);
  tty->t_line.ln_count = 0;
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
      tty->t_rocount = 0;
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

static void tty_reinput(tty_t *tty) {
  /* This is a bit hacky, but it works. */
  ringbuf_t old_inq = tty->t_inq;
  tty->t_inq.count = 0;
  tty->t_inq.head = tty->t_inq.tail;
  uint8_t c;
  while (ringbuf_getb(&old_inq, &c)) {
    tty_input(tty, c);
  }
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
      bool reinput = false; /* Process pending input again. */

      if (cmd == TIOCSETAW || cmd == TIOCSETAF) {
        /* TODO Drain all output. */
      }

      if (cmd == TIOCSETAF) {
        /* Discard all pending input. */
        tty_discard_input(tty);
      } else {
        /* If we didn't discard the input and we're changing from
         * raw mode to canonical mode or vice versa, we need to handle
         * characters that the user has already typed appropriately. */
        if ((t->c_lflag & ICANON) != (tty->t_lflag & ICANON)) {
          if (t->c_lflag & ICANON) {
            /* Switching from raw to canonical mode.
             * Process all pending input again, this time in canonical mode.
             * Defer until we have made the changes to termios. */
            reinput = true;
          } else {
            /* Switching from canonical to raw mode.
             * Transfer characters from the unfinished line
             * to the input queue and wake up readers. */
            tty_line_finish(tty);
            tty_wakeup(tty);
          }
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

      if (reinput)
        tty_reinput(tty);
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
