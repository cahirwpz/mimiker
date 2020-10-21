#ifndef _SYS_TTY_H_
#define _SYS_TTY_H_

#include <sys/termios.h>
#include <sys/vnode.h>
#include <sys/ringbuf.h>
#include <sys/condvar.h>
#include <sys/mutex.h>

#define TTY_QUEUE_SIZE 0x400
#define TTY_OUT_LOW_WATER (TTY_QUEUE_SIZE / 4)
#define LINEBUF_SIZE 0x100

struct tty;

typedef void (*t_notify_out_t)(struct tty *);

/* Routines supplied by the serial device driver. */
typedef struct {
  /* Called when new characters are added to the tty's output queue.
   * This routine should make sure that at some point in the future all
   * characters in the tty's output queue at the time of the call will
   * be written to the device. */
  t_notify_out_t t_notify_out;
} ttyops_t;

/* Line buffer */
typedef struct {
  uint8_t *ln_buf;
  size_t ln_size;  /* Capacity */
  size_t ln_count; /* Number of characters in the buffer */
} linebuf_t;

typedef struct tty {
  mtx_t t_lock;
  uint32_t t_flags;
  ringbuf_t t_inq;   /* Input queue */
  condvar_t t_incv;  /* CV for readers waiting for input */
  ringbuf_t t_outq;  /* Output queue */
  condvar_t t_outcv; /* CV for threads waiting for space in outq */
  linebuf_t t_line;  /* Line buffer */
  size_t t_column;   /* Cursor's column position */
  /* TODO explain `t_rocol` and `t_rocount`  */
  size_t t_rocol, t_rocount;
  ttyops_t t_ops; /* Serial device operations */
  struct termios t_termios;
  void *t_data; /* Serial device driver's private data */
} tty_t;

#define t_cc t_termios.c_cc
#define t_cflag t_termios.c_cflag
#define t_iflag t_termios.c_iflag
#define t_ispeed t_termios.c_ispeed
#define t_lflag t_termios.c_lflag
#define t_oflag t_termios.c_oflag
#define t_ospeed t_termios.c_ospeed

/* TTY flags */
#define TTY_WAIT_OUT_LOWAT 0x1 /* Someone is waiting for space in outq */
#define TTY_WAIT_DRAIN_OUT 0x2 /* Someone is waiting for outq to drain */

extern vnodeops_t tty_vnodeops;

/*
 * Allocate and initialize a new `tty` structure.
 */
tty_t *tty_alloc(void);

/*
 * Put a single character into the tty's input queue, provided it's not full.
 * Must be called with tty->t_lock held.
 */
void tty_input(tty_t *tty, uint8_t c);

/*
 * Wake up threads waiting for space in the output queue.
 * Must be called by drivers after consuming one or more characters
 * from the tty's output queue.
 * Must be called with tty->t_lock held.
 */
void tty_getc_done(tty_t *tty);

#endif /* !_SYS_TTY_H_ */
