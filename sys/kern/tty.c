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
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/signal.h>
#include <sys/devfs.h>
#include <sys/file.h>
#include <sys/filio.h>

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
  (ECHOKE | ECHOE | ECHOK | ECHO | ECHONL | ECHOCTL | ICANON | ISIG | TOSTOP | \
   NOFLSH)

static bool tty_output(tty_t *tty, uint8_t c);
static void tty_discard_input(tty_t *tty);
static void tty_discard_output(tty_t *tty);

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
  tty_t *tty = kmalloc(M_DEV, sizeof(tty_t), M_WAITOK | M_ZERO);
  mtx_init(&tty->t_lock, 0);
  ringbuf_init(&tty->t_inq, kmalloc(M_DEV, TTY_QUEUE_SIZE, M_WAITOK),
               TTY_QUEUE_SIZE);
  cv_init(&tty->t_incv, "t_incv");
  ringbuf_init(&tty->t_outq, kmalloc(M_DEV, TTY_QUEUE_SIZE, M_WAITOK),
               TTY_QUEUE_SIZE);
  cv_init(&tty->t_outcv, "t_outcv");
  cv_init(&tty->t_serialize_cv, "t_serialize_cv");
  tty->t_line.ln_buf = kmalloc(M_DEV, LINEBUF_SIZE, M_WAITOK);
  tty->t_line.ln_size = LINEBUF_SIZE;
  tty->t_line.ln_count = 0;
  tty_init_termios(&tty->t_termios);
  return tty;
}

void tty_free(tty_t *tty) {
  assert(!tty_opened(tty));
  assert(tty_detached(tty));
  assert(!mtx_owned(&tty->t_lock));
  mtx_destroy(&tty->t_lock);
  cv_destroy(&tty->t_incv);
  cv_destroy(&tty->t_outcv);
  cv_destroy(&tty->t_serialize_cv);
  kfree(M_DEV, tty->t_line.ln_buf);
  kfree(M_DEV, tty->t_inq.data);
  kfree(M_DEV, tty->t_outq.data);
  kfree(M_DEV, tty);
}

static int tty_wait(tty_t *tty, condvar_t *cv) {
  assert(mtx_owned(&tty->t_lock));
  assert(!tty_detached(tty));

  int error;

  error = cv_wait_intr(cv, &tty->t_lock);

  if (tty_detached(tty))
    return ENXIO;

  /* Did we receive a signal while sleeping? */
  if (error == EINTR)
    return ERESTARTSYS;

  return error;
}

/* Notify the serial device driver that there are characters
 * in the output queue. */
static void tty_notify_out(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  if (tty->t_outq.count > 0) {
    tty->t_ops.t_notify_out(tty);
  }
}

static void tty_notify_in(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  if (tty->t_ops.t_notify_in)
    tty->t_ops.t_notify_in(tty);
}

static void tty_notify_active(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  if (tty->t_ops.t_notify_active)
    tty->t_ops.t_notify_active(tty);
}

static void tty_notify_inactive(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  if (tty->t_ops.t_notify_inactive)
    tty->t_ops.t_notify_inactive(tty);
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
 * Put characters from buf directly in the output queue.
 * If there's not enough space for all characters, no characters are written
 * to the queue and false is returned. Returns true on success.
 */
static bool tty_outq_write(tty_t *tty, uint8_t *buf, size_t len) {
  if (tty->t_lflag & FLUSHO)
    return true;
  return ringbuf_putnb(&tty->t_outq, buf, len);
}

/*
 * Put a character directly in the output queue.
 * Returns false if there's no space left in the queue, otherwise returns true.
 */
static bool tty_outq_putc(tty_t *tty, uint8_t c) {
  if (tty->t_lflag & FLUSHO)
    return true;
  return ringbuf_putb(&tty->t_outq, c);
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

/*
 * Check whether we're allowed to continue with an operation.
 * Heavily based on FreeBSD's tty_wait_background().
 */
static int tty_check_background(tty_t *tty, int sig) {
  thread_t *td = thread_self();
  proc_t *p = td->td_proc;
  pgrp_t *pg;

  assert(sig == SIGTTIN || sig == SIGTTOU);
  assert(mtx_owned(&tty->t_lock));

tryagain:
  WITH_PROC_LOCK(p) {
    /*
     * The current process group should only get the signal, when:
     * - This terminal is the controlling terminal
     * - The process group is not the foreground process group
     * - The signal to send to the process isn't masked
     * - Its process group is not orphaned
     */
    if (!tty_is_ctty(tty, p) || p->p_pgrp == tty->t_pgrp)
      return 0;

    if (p->p_sigactions[sig].sa_handler == SIG_IGN ||
        __sigismember(&td->td_sigmask, sig))
      return (sig == SIGTTOU ? 0 : EIO);

    pg = p->p_pgrp;
  }

  WITH_MTX_LOCK (&pg->pg_lock) {
    /* Our process group might have changed: check. */
    if (p->p_pgrp != pg)
      goto tryagain;

    /* Unlocked read of pg_jobc: not a big deal. */
    if (pg->pg_jobc == 0)
      return EIO;

    sig_pgkill(pg, &DEF_KSI_RAW(sig));
  }

  /* Handle the signal at userspace boundary. */
  return ERESTARTSYS;
}

static void tty_in_hiwat(tty_t *tty) {
  tty->t_flags |= TF_IN_HIWAT;

  if (tty->t_iflag & IMAXBEL) {
    tty_output(tty, CTRL('g'));
    tty_notify_out(tty);
  }
}

bool tty_input(tty_t *tty, uint8_t c) {
  int iflag = tty->t_iflag;
  int lflag = tty->t_lflag;
  uint8_t *cc = tty->t_cc;

  if (c == '\r') {
    if (iflag & IGNCR)
      return true;
    else if (iflag & ICRNL)
      c = '\n';
  } else if (c == '\n' && (iflag & INLCR)) {
    c = '\r';
  }

  if (lflag & ISIG) {
    signo_t signal = 0;
    /* Signal processing. */
    if (CCEQ(cc[VINTR], c)) {
      signal = SIGINT;
    } else if (CCEQ(cc[VQUIT], c)) {
      signal = SIGQUIT;
    } else if (CCEQ(cc[VSUSP], c)) {
      signal = SIGTSTP;
    }

    if (signal) {
      if (!(lflag & NOFLSH)) {
        tty_discard_input(tty);
        tty_discard_output(tty);
      }
      tty_echo(tty, c);
      pgrp_t *pg = tty->t_pgrp;
      if (pg)
        WITH_MTX_LOCK (&pg->pg_lock)
          sig_pgkill(pg, &DEF_KSI_RAW(signal));
      tty_notify_out(tty);
      return true;
    }
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
      return true;
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
      return true;
    }

    bool is_break = tty_is_break(tty, c);

    /* Check for possibility of overflow. */
    if ((tty->t_inq.count + tty->t_line.ln_count >= TTY_QUEUE_SIZE - 1 ||
         tty->t_line.ln_count == LINEBUF_SIZE - 1) &&
        !is_break) {
      tty_in_hiwat(tty);
      return false;
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
    return true;
  } else {
    /* Raw (non-canonical) mode */
    if (tty->t_inq.count >= TTY_QUEUE_SIZE) {
      tty_in_hiwat(tty);
      return false;
    }

    ringbuf_putb(&tty->t_inq, c);
    tty_wakeup(tty);
    return true;
  }
}

static void tty_check_in_lowat(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));

  if (!(tty->t_flags & TF_IN_HIWAT))
    return;

  if (tty->t_inq.count < TTY_IN_LOW_WATER) {
    tty->t_flags &= ~TF_IN_HIWAT;
    tty_notify_in(tty);
  }
}

static int tty_do_read(file_t *f, uio_t *uio) {
  tty_t *tty = f->f_data;
  size_t start_resid = uio->uio_resid;
  int error = 0;

  uint8_t c;
  WITH_MTX_LOCK (&tty->t_lock) {

    if (tty_detached(tty))
      return ENXIO;

    while (true) {
      if ((error = tty_check_background(tty, SIGTTIN)))
        return error;

      if (tty->t_inq.count > 0)
        break;

      if ((error = tty_wait(tty, &tty->t_incv)))
        return error;
      /* The foreground process group may have changed while
       * we were sleeping, so go to the beginning of the loop
       * and check again. */
    }

    if (tty->t_lflag & ICANON) {
      /* In canonical mode, read as many characters as possible, but no more
       * than one line. */
      while (ringbuf_getb(&tty->t_inq, &c)) {
        if (CCEQ(tty->t_cc[VEOF], c))
          break;
        if ((error = uiomove(&c, 1, uio)))
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

    tty_check_in_lowat(tty);
  }

  /* Don't report errors on partial reads. */
  if (start_resid > uio->uio_resid)
    error = 0;

  return error;
}

static int tty_read(file_t *f, uio_t *uio) {
  int error;

  uio->uio_offset = 0; /* This device does not support offsets. */

  error = tty_do_read(f, uio);

  /* Report EOF instead of error if the driver is detached. */
  return (error == ENXIO ? 0 : error);
}

static void tty_discard_input(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  ringbuf_reset(&tty->t_inq);
  tty->t_line.ln_count = 0;
  tty_check_in_lowat(tty);
}

static void tty_discard_output(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  ringbuf_reset(&tty->t_outq);
  cv_broadcast(&tty->t_outcv);
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
static bool tty_output(tty_t *tty, uint8_t c) {
  assert(mtx_owned(&tty->t_lock));

  int oflag = tty->t_oflag;
  /* Check if output processing is enabled. */
  if (!(oflag & OPOST)) {
    return tty_outq_putc(tty, c);
  }

  uint8_t cb[2];
  cb[0] = c;
  int ccount = tty_process_out(tty, oflag, cb);
  int col = tty->t_column;

  if (!tty_outq_write(tty, cb, ccount))
    return false;

  for (int i = 0; i < ccount; i++) {
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
  return true;
}

/*
 * Write a single character to the terminal.
 * If it can't immediately write the character due to lack of space
 * in the output queue, it goes to sleep waiting for space to become available.
 */
static int tty_output_sleep(tty_t *tty, uint8_t c) {
  int error;
  while (!tty_output(tty, c)) {
    tty_notify_out(tty);
    /* tty_notify_out() can synchronously write characters to the device,
     * so it may have written enough characters for us not to need to sleep. */
    if (tty->t_outq.count < TTY_OUT_LOW_WATER)
      continue;
    tty->t_flags |= TF_WAIT_OUT_LOWAT;
    if ((error = tty_wait(tty, &tty->t_outcv)))
      return error;
  }
  return 0;
}

static int tty_do_write(tty_t *tty, uio_t *uio) {
  uint8_t c;
  size_t start_resid = uio->uio_resid;
  int error = 0;

  while (uio->uio_resid > 0) {
    if ((error = uiomove(&c, 1, uio)))
      break;
    if ((error = tty_output_sleep(tty, c)))
      break;
    tty->t_rocount = 0;
  }
  tty_notify_out(tty);

  /* Don't report errors on partial writes. */
  if (start_resid > uio->uio_resid)
    error = 0;

  return error;
}

static int tty_write(file_t *f, uio_t *uio) {
  tty_t *tty = f->f_data;
  int error = 0;

  uio->uio_offset = 0; /* This device does not support offsets. */

  WITH_MTX_LOCK (&tty->t_lock) {
    if (tty_detached(tty))
      return ENXIO;

    if (tty->t_lflag & TOSTOP) {
      /* Wait until we're the foreground process group. */
      if ((error = tty_check_background(tty, SIGTTOU)))
        return error;
    }

    /* Wait for our turn.
     * We need to use TF_OUT_BUSY to serialize calls to tty_write(),
     * since tty_do_write() may sleep, releasing the tty lock. */
    while (tty->t_flags & TF_OUT_BUSY)
      if ((error = tty_wait(tty, &tty->t_serialize_cv)))
        return error;
    tty->t_flags |= TF_OUT_BUSY;

    error = tty_do_write(tty, uio);

    tty->t_flags &= ~TF_OUT_BUSY;
    cv_signal(&tty->t_serialize_cv);
  }

  return error;
}

void tty_getc_done(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));

  size_t cnt = tty->t_outq.count;
  tty_flags_t oldf = tty->t_flags;

  if (cnt < TTY_OUT_LOW_WATER)
    tty->t_flags &= ~TF_WAIT_OUT_LOWAT;
  if (cnt == 0)
    tty->t_flags &= ~TF_WAIT_DRAIN_OUT;

  if (tty->t_flags != oldf)
    cv_broadcast(&tty->t_outcv);
}

static int tty_drain_out(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  int error;

  while (tty->t_outq.count > 0) {
    tty->t_flags |= TF_WAIT_DRAIN_OUT;
    if ((error = tty_wait(tty, &tty->t_outcv)))
      return error;
  }
  return 0;
}

static int tty_vn_close(vnode_t *v, file_t *fp) {
  tty_t *tty = devfs_node_data(v);
  SCOPED_MTX_LOCK(&tty->t_lock);
  assert(tty->t_opencount > 0);
  tty->t_opencount--;
  if (tty->t_opencount == 0)
    tty_notify_inactive(tty);
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

static int tty_get_termios(tty_t *tty, struct termios *t) {
  SCOPED_MTX_LOCK(&tty->t_lock);
  if (tty_detached(tty))
    return ENXIO;
  memcpy(t, &tty->t_termios, sizeof(struct termios));
  return 0;
}

static int tty_set_termios(tty_t *tty, u_long cmd, struct termios *t) {
  int error;

  SCOPED_MTX_LOCK(&tty->t_lock);
  if (tty_detached(tty))
    return ENXIO;

  if (cmd == TIOCSETAW || cmd == TIOCSETAF)
    if ((error = tty_drain_out(tty)))
      return error;
  if (cmd == TIOCSETAF)
    tty_discard_input(tty);

  bool reinput = false; /* Process pending input again. */

  if (cmd != TIOCSETAF) {
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
  return 0;
}

static int tty_get_fg_pgrp(tty_t *tty, int *pgid_p) {
  SCOPED_MTX_LOCK(&tty->t_lock);
  if (tty_detached(tty))
    return ENXIO;
  proc_t *p = proc_self();
  if (tty->t_session != p->p_pgrp->pg_session)
    return ENOTTY;
  if (tty->t_pgrp)
    *pgid_p = tty->t_pgrp->pg_id;
  else
    /* Return a value greater than 1 that's not equal to any PGID. */
    *pgid_p = INT_MAX;
  return 0;
}

static int tty_set_fg_pgrp(tty_t *tty, pgid_t pgid) {
  WITH_MTX_LOCK (all_proc_mtx) {
    proc_t *p = proc_self();
    pgrp_t *pg = pgrp_lookup(pgid);
    WITH_MTX_LOCK (&tty->t_lock) {
      if (tty_detached(tty))
        return ENXIO;
      /* The target process group must be in the same session
       * as the calling process. */
      if (pg == NULL || pg->pg_session != p->p_pgrp->pg_session)
        return EPERM;
      /* Calling process must be controlled by the terminal. */
      if (tty->t_session != p->p_pgrp->pg_session)
        return ENOTTY;
      tty->t_pgrp = pg;
    }
  }
  return 0;
}

static int tty_set_winsize(tty_t *tty, struct winsize *sz) {
  SCOPED_MTX_LOCK(&tty->t_lock);
  if (memcmp(&tty->t_winsize, sz, sizeof(struct winsize)) == 0)
    return 0;
  tty->t_winsize = *sz;
  /* Send SIGWINCH to the foreground process group. */
  pgrp_t *pgrp = tty->t_pgrp;
  if (pgrp) {
    SCOPED_MTX_LOCK(&pgrp->pg_lock);
    sig_pgkill(pgrp, &DEF_KSI_RAW(SIGWINCH));
  }
  return 0;
}

static int tty_ioctl(file_t *f, u_long cmd, void *data) {
  tty_t *tty = f->f_data;

  switch (cmd) {
    case FIONREAD: {
      SCOPED_MTX_LOCK(&tty->t_lock);
      *(int *)data = tty->t_inq.count;
      return 0;
    }
    case TIOCGETA:
      return tty_get_termios(tty, (struct termios *)data);
    case TIOCSETA:  /* Set termios immediately */
    case TIOCSETAW: /* Set termios after waiting for output to drain */
    case TIOCSETAF: /* Like TIOCSETAW, but also discard all pending input */
      return tty_set_termios(tty, cmd, (struct termios *)data);
    case TIOCGPGRP:
      return tty_get_fg_pgrp(tty, data);
    case TIOCSPGRP:
      return tty_set_fg_pgrp(tty, *(pgid_t *)data);
    case TIOCGWINSZ: {
      SCOPED_MTX_LOCK(&tty->t_lock);
      *(struct winsize *)data = tty->t_winsize;
      return 0;
    }
    case TIOCSWINSZ:
      return tty_set_winsize(tty, data);
    case 0:
      return EPASSTHROUGH;
    default: {
      char buf[32];
      IOCSNPRINTF(buf, sizeof(buf), cmd);
      klog("Unsupported tty ioctl: %s", buf);
      unsigned len = IOCPARM_LEN(cmd);
      memset(data, 0, len);
      return 0;
    }
  }
}

static void tty_hangup(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));

  /* CLOCAL means we should ignore modem status changes. */
  if (!tty_opened(tty) || (tty->t_lflag & CLOCAL))
    return;

  session_t *s = tty->t_session;

  /* Send SIGHUP to session leader.
   * NOTE: access to s_leader is safe here,
   * even though we're not holding all_proc_mtx. See session_leave(). */
  if (s && s->s_leader) {
    proc_t *leader = s->s_leader;
    WITH_PROC_LOCK(leader) {
      sig_kill(leader, &DEF_KSI_RAW(SIGHUP));
    }
  }

  tty_discard_input(tty);
}

void tty_detach_driver(tty_t *tty) {
  assert(mtx_owned(&tty->t_lock));
  assert(!tty_detached(tty));

  /* Simulate a modem hangup. */
  tty_hangup(tty);

  tty->t_flags |= TF_DRIVER_DETACHED;

  /* Wake up sleepers so that they abort. */
  cv_broadcast(&tty->t_incv);
  cv_broadcast(&tty->t_outcv);
  cv_broadcast(&tty->t_serialize_cv);

  /* We can't free the tty structure yet, as there may still be existing
   * references to the vnode. We free it in tty_vn_reclaim, once all references
   * to the vnode are gone. */
  devfs_unlink(tty->t_vnode->v_data);
  vnode_t *v = tty->t_vnode;
  tty->t_vnode = NULL;
  /* Unlock the tty before dropping our reference to the vnode,
   * since we don't want to free the tty while holding its lock! */
  mtx_unlock(&tty->t_lock);
  vnode_drop(v);
}

/* We implement I/O operations as fileops in order to bypass
 * the vnode layer's locking. */
static fileops_t tty_fileops = {
  .fo_read = tty_read,
  .fo_write = tty_write,
  .fo_close = default_vnclose,
  .fo_seek = default_vnseek,
  .fo_stat = default_vnstat,
  .fo_ioctl = tty_ioctl,
};

/* If the process is a session leader, the session has no associated terminal,
 * and the terminal has no associated session, make this terminal
 * the controlling terminal for the session. */
static void maybe_assoc_ctty(proc_t *p, tty_t *tty) {
  assert(mtx_owned(all_proc_mtx));
  assert(mtx_owned(&tty->t_lock));

  if (!proc_is_session_leader(p))
    return;
  if (tty->t_session != NULL)
    return;
  session_t *s = p->p_pgrp->pg_session;
  if (s->s_tty != NULL)
    return;
  tty->t_session = s;
  s->s_tty = tty;
  tty->t_pgrp = p->p_pgrp;
}

static int _tty_vn_open(vnode_t *v, int mode, file_t *fp) {
  tty_t *tty = devfs_node_data(v);
  proc_t *p = proc_self();
  int error;

  WITH_MTX_LOCK (&tty->t_lock) {
    if (tty_detached(tty))
      return ENXIO;

    if ((error = vnode_open_generic(v, mode, fp)) != 0)
      return error;

    maybe_assoc_ctty(p, tty);

    tty->t_opencount++;

    if (tty->t_opencount == 1)
      tty_notify_active(tty);
  }

  fp->f_ops = &tty_fileops;
  fp->f_data = tty;
  return error;
}

static int tty_vn_open(vnode_t *v, int mode, file_t *fp) {
  SCOPED_MTX_LOCK(all_proc_mtx);
  return _tty_vn_open(v, mode, fp);
}

static int tty_vn_reclaim(vnode_t *v) {
  tty_free(devfs_node_data(v));
  devfs_free(v->v_data);
  return 0;
}

static vnodeops_t tty_vnodeops = {
  .v_open = tty_vn_open,
  .v_close = tty_vn_close,
  .v_reclaim = tty_vn_reclaim,
};

int tty_makedev(devfs_node_t *parent, const char *name, tty_t *tty) {
  return devfs_makedev(parent, name, &tty_vnodeops, tty, &tty->t_vnode);
}

/* Controlling terminal pseudo-device (/dev/tty) */

static int dev_tty_open(vnode_t *v, int mode, file_t *fp) {
  proc_t *p = proc_self();

  SCOPED_MTX_LOCK(all_proc_mtx);
  tty_t *tty = p->p_pgrp->pg_session->s_tty;
  if (!tty)
    return ENXIO;

  return _tty_vn_open(tty->t_vnode, mode, fp);
}

static vnodeops_t dev_tty_vnodeops = {.v_open = dev_tty_open};

static void init_dev_tty(void) {
  devfs_makedev(NULL, "tty", &dev_tty_vnodeops, NULL, NULL);
}

SET_ENTRY(devfs_init, init_dev_tty);
