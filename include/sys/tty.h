#ifndef _SYS_TTY_H_
#define _SYS_TTY_H_

#include <sys/termios.h>
#include <sys/vnode.h>
#include <sys/ringbuf.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/proc.h>

#define TTY_QUEUE_SIZE 0x400
#define TTY_OUT_LOW_WATER (TTY_QUEUE_SIZE / 4)
#define TTY_IN_LOW_WATER (TTY_QUEUE_SIZE / 4)
#define LINEBUF_SIZE 0x100

typedef struct session session_t;

struct tty;

typedef void (*t_notify_out_t)(struct tty *);
typedef void (*t_notify_in_t)(struct tty *);

/* Routines supplied by the serial device driver. */
typedef struct {
  /* Called when new characters are added to the tty's output queue.
   * This routine should make sure that at some point in the future all
   * characters in the tty's output queue at the time of the call will
   * be written to the device. */
  t_notify_out_t t_notify_out;
  /* Called when space becomes available in the tty's input queue.
   * This function will be called only after tty_input() fails to put the
   * received character in the input queue. */
  t_notify_in_t t_notify_in;
} ttyops_t;

/* TTY flags */
typedef enum {
  TF_WAIT_OUT_LOWAT = 0x1, /* Someone is waiting for space in outq */
  TF_WAIT_DRAIN_OUT = 0x2, /* Someone is waiting for outq to drain */
  TF_OUT_BUSY = 0x4,       /* Serialization of write() calls:
                            * a thread is currently writing to this TTY. */
  TF_IN_HIWAT = 0x8,       /* Input high watermark reached. Once space becomes
                            * available in the inq, call t_notify_in(). */
} tty_flags_t;

/* Line buffer */
typedef struct {
  uint8_t *ln_buf;
  size_t ln_size;  /* Capacity */
  size_t ln_count; /* Number of characters in the buffer */
} linebuf_t;

typedef struct tty {
  mtx_t t_lock;
  tty_flags_t t_flags;
  ringbuf_t t_inq;           /* Input queue */
  condvar_t t_incv;          /* CV for readers waiting for input */
  ringbuf_t t_outq;          /* Output queue */
  condvar_t t_outcv;         /* CV for threads waiting for space in outq */
  linebuf_t t_line;          /* Line buffer */
  size_t t_column;           /* Cursor's column position */
  size_t t_rocol, t_rocount; /* See explanation below */
  condvar_t t_serialize_cv;  /* CV used to serialize write() calls */
  condvar_t t_background_cv; /* Background wait CV */
  ttyops_t t_ops;            /* Serial device operations */
  struct termios t_termios;
  pgrp_t *t_pgrp;       /* Foreground process group */
  session_t *t_session; /* Session controlled by this tty */
  vnode_t *t_vnode;     /* Device vnode */
  void *t_data;         /* Serial device driver's private data */
} tty_t;

/*
 * Explanation of t_rocol and t_rocount:
 * In canonical mode, the line that the user is typing can be echoed back
 * to the serial device, so that the user knows what they're typing.
 * However, processes can still write to the terminal, so the echoed line
 * can be interleaved with output from processes.
 * When the user wants to erase characters from the line, we only want to
 * rub out (i.e. make no longer visible) characters that the user has typed,
 * but we must preserve characters output by processes.
 * For example, suppose the user types 'foo', a process outputs 'qux', and
 * then the user types 'bar'. The line on the display device should look like
 * this:
 * fooquxbar
 * Note that the contents of the line buffer are 'foobar'.
 * We can safely erase 'bar' from the line, since it's uninterrupted by output
 * from processes. After the user presses the erase key 3 times, the line on the
 * display device will look like this:
 * fooqux
 * Now, what should happen when the user presses the erase key? We must preserve
 * the 'qux' output by the process, since that's not recoverable. We therefore
 * re-echo the contents of the line buffer (with the last character removed) to
 * the screen:
 * fooqux
 * fo
 * Clearly, we need to be able to tell when it's safe to rub out the last
 * character in the line buffer. More precisely, we need to know whether the
 * last character on the line displayed to the user comes from the user
 * or from a process. We therefore keep track of the longest suffix
 * of the line buffer that was echoed to the screen uninterrupted.
 * The length of that suffix is stored in t_rocount, and the column index
 * of the first character in the suffix is stored in t_rocol.
 * When a character typed by the user is echoed, we increment t_rocount.
 * When a process outputs any character, t_rocount is set to 0.
 * Erasing a character decrements t_rocount.
 * When t_rocount > 0, we know we can safely erase the last character
 * in the line buffer. When t_rocount == 0, we must re-echo the line buffer.
 */

#define t_cc t_termios.c_cc
#define t_cflag t_termios.c_cflag
#define t_iflag t_termios.c_iflag
#define t_ispeed t_termios.c_ispeed
#define t_lflag t_termios.c_lflag
#define t_oflag t_termios.c_oflag
#define t_ospeed t_termios.c_ospeed

extern vnodeops_t tty_vnodeops;

/*
 * Allocate and initialize a new `tty` structure.
 */
tty_t *tty_alloc(void);

/*
 * Put a single character into the tty's input queue, provided it's not full.
 * Must be called with tty->t_lock held.
 * Returns false if there's no space in the tty's input queue, true on success.
 */
bool tty_input(tty_t *tty, uint8_t c);

/*
 * Wake up threads waiting for space in the output queue.
 * Must be called by drivers after consuming one or more characters
 * from the tty's output queue.
 * Must be called with tty->t_lock held.
 */
void tty_getc_done(tty_t *tty);

/*
 * Returns whether `tty` is the controlling terminal of process `p`.
 * Must be called with `tty->t_lock` and `p->p_lock` held.
 */
static inline bool tty_is_ctty(tty_t *tty, proc_t *p) {
  assert(mtx_owned(&tty->t_lock));
  assert(mtx_owned(&p->p_lock));
  return (tty->t_session == p->p_pgrp->pg_session);
}

#endif /* !_SYS_TTY_H_ */
