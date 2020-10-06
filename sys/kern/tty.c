#define KL_LOG KL_TTY
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/malloc.h>
#include <sys/kmem_flags.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/ringbuf.h>
#include <sys/vnode.h>
#include <sys/stat.h>

tty_t *tty_alloc(void) {
  /* XXX: What pool should we use here? */
  tty_t *tty = kmalloc(M_TEMP, sizeof(tty_t), M_WAITOK);
  mtx_init(&tty->t_lock, 0);
  ringbuf_init(&tty->t_inq, kmalloc(M_TEMP, TTY_QUEUE_SIZE, M_WAITOK),
               TTY_QUEUE_SIZE);
  cv_init(&tty->t_incv, "t_incv");
  ringbuf_init(&tty->t_outq, kmalloc(M_TEMP, TTY_QUEUE_SIZE, M_WAITOK),
               TTY_QUEUE_SIZE);
  cv_init(&tty->t_outcv, "t_outcv");
  tty->t_ops.t_output = NULL;
  tty->t_data = NULL;

  return tty;
}

void tty_input(tty_t *tty, uint8_t c) {
  /* TODO: Canonical mode, character processing. */
  if (ringbuf_putb(&tty->t_inq, c)) {
    cv_signal(&tty->t_incv);
  } else {
    klog("TTY input queue full when trying to insert character 0x%hhx", c);
  }
}

static int tty_read(vnode_t *v, uio_t *uio, int ioflags) {
  tty_t *tty = v->v_data;
  int error;

  uio->uio_offset = 0; /* This device does not support offsets. */

  uint8_t byte;
  WITH_MTX_LOCK (&tty->t_lock) {
    while (!ringbuf_getb(&tty->t_inq, &byte))
      cv_wait(&tty->t_incv, &tty->t_lock);
  }
  /* XXX: This is not how reading characters in raw mode should work,
   *      but this is a truly minimal implementation. */
  if ((error = uiomove_frombuf(&byte, 1, uio)))
    return error;

  return 0;
}

static int tty_write(vnode_t *v, uio_t *uio, int ioflags) {
  tty_t *tty = v->v_data;
  int error;

  uio->uio_offset = 0; /* This device does not support offsets. */

  while (uio->uio_resid > 0) {
    error = ringbuf_write(&tty->t_outq, uio);
    if (tty->t_outq.count > 0)
      tty->t_ops.t_output(tty);
    if (error)
      return error;
  }

  return 0;
}

static int tty_close(vnode_t *v, file_t *fp) {
  /* TODO implement. */
  return 0;
}

static int tty_ioctl(vnode_t *v, u_long cmd, void *data) {
  /* TODO implement. */
  if (cmd) {
    unsigned len = IOCPARM_LEN(cmd);
    memset(data, 0, len);
    return 0;
  }

  return EPASSTHROUGH;
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
