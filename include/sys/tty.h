#ifndef _SYS_TTY_H_
#define _SYS_TTY_H_

#include <sys/vnode.h>
#include <sys/ringbuf.h>
#include <sys/condvar.h>
#include <sys/mutex.h>

#define TTY_QUEUE_SIZE 0x400

struct tty;

typedef void (*t_output_t)(struct tty *);

typedef struct {
  t_output_t t_output;
} ttyops_t;

typedef struct tty {
  mtx_t t_lock;
  ringbuf_t t_inq;
  condvar_t t_incv;
  ringbuf_t t_outq;
  condvar_t t_outcv;
  ttyops_t t_ops;
  void *t_data;
} tty_t;

extern vnodeops_t tty_vnodeops;

/*
 * Allocate and initialize a new `tty` structure.
 */
tty_t *tty_alloc(void);

/*
 * Put a single character into the tty's input queue, provided it's not full.
 * Must be called with `t_lock` acquired.
 */
void tty_input(tty_t *tty, uint8_t c);

#endif /* !_SYS_TTY_H_ */
