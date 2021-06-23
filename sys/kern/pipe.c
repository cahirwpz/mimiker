#define KL_LOG KL_FILE
#include <sys/klog.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/errno.h>
#include <sys/pipe.h>
#include <sys/libkern.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/ringbuf.h>
#include <sys/uio.h>

typedef struct pipe pipe_t;

struct pipe {
  mtx_t mtx; /*!< protects all other fields */
  /* Pipe can be freed when both ends are closed */
  bool producer_closed; /*!< write end closed */
  bool consumer_closed; /*!< read end closed */
  condvar_t nonempty;   /*!< used to wait data to appear in the buffer */
  condvar_t nonfull;    /*!< used to wait for free space in the buffer */
  ringbuf_t buf;        /*!< buffer with pipe data */
};

static POOL_DEFINE(P_PIPE, "pipe", sizeof(pipe_t));

static pipe_t *pipe_alloc(void) {
  pipe_t *pipe = pool_alloc(P_PIPE, M_ZERO);
  mtx_init(&pipe->mtx, 0);
  pipe->producer_closed = false;
  pipe->consumer_closed = false;
  cv_init(&pipe->nonempty, "pipe_nonempty");
  cv_init(&pipe->nonfull, "pipe_nonfull");
  pipe->buf.data = kmem_alloc(PIPE_SIZE, M_ZERO);
  pipe->buf.size = PIPE_SIZE;
  return pipe;
}

static void pipe_free(pipe_t *pipe) {
  kmem_free(pipe->buf.data, PIPE_SIZE);
  pool_free(P_PIPE, pipe);
}

static int pipe_read(file_t *f, uio_t *uio) {
  pipe_t *pipe = f->f_data;

  assert(!pipe->consumer_closed);

  /* user requested read of 0 bytes */
  if (uio->uio_resid == 0)
    return 0;

  /* no read atomicity for now! */
  WITH_MTX_LOCK (&pipe->mtx) {
    while (ringbuf_empty(&pipe->buf)) {
      if (pipe->producer_closed)
        /* pipe empty, no producers, return end-of-file */
        return 0;
      /* restart the syscall if we were interrupted by a signal */
      if (cv_wait_intr(&pipe->nonempty, &pipe->mtx))

        return ERESTARTSYS;
    }

    int res = ringbuf_read(&pipe->buf, uio);
    if (res)
      return res;
    /* notify producer that free space is available */
    cv_broadcast(&pipe->nonfull);
  }

  return 0;
}

static int pipe_write(file_t *f, uio_t *uio) {
  pipe_t *pipe = f->f_data;
  int res;

  assert(!pipe->producer_closed);

  if (uio->uio_resid == 0)
    return 0;

  size_t old_resid = uio->uio_resid;

  /* no write atomicity for now! */
  WITH_MTX_LOCK (&pipe->mtx) {
    while (true) {
      if (pipe->consumer_closed) {
        res = EPIPE;
        break;
      }
      res = ringbuf_write(&pipe->buf, uio);
      if (res)
        break;
      /* notify consumer that new data is available */
      cv_broadcast(&pipe->nonempty);
      /* nothing left to write? */
      if (uio->uio_resid == 0)
        return 0;
      /* buffer is full so wait for some data to be consumed */
      if (cv_wait_intr(&pipe->nonfull, &pipe->mtx)) {
        res = ERESTARTSYS;
        break;
      }
    }
  }

  /* don't report errors on partial writes */
  if (uio->uio_resid < old_resid)
    res = 0;

  return res;
}

static int pipe_close(file_t *f) {
  pipe_t *pipe = f->f_data;
  bool can_free;

  WITH_MTX_LOCK (&pipe->mtx) {
    /* If we're a consumer, the file is read-only, otherwise it's write-only. */
    if (f->f_flags & FF_READ) {
      assert(!pipe->consumer_closed);
      pipe->consumer_closed = true;
      /* Wake up producers so that they exit. */
      cv_broadcast(&pipe->nonfull);
    } else {
      assert(!pipe->producer_closed);
      pipe->producer_closed = true;
      /* Wake up consumers so that they exit. */
      cv_broadcast(&pipe->nonempty);
    }
    can_free = pipe->consumer_closed && pipe->producer_closed;
  }

  /* Free the pipe if both ends have been closed. */
  if (can_free)
    pipe_free(pipe);

  return 0;
}

static int pipe_stat(file_t *f, stat_t *sb) {
  return EOPNOTSUPP;
}

static int pipe_ioctl(file_t *f, u_long cmd, void *data) {
  return EOPNOTSUPP;
}

static int pipe_seek(file_t *f, off_t offset, int whence, off_t *newoffp) {
  return EOPNOTSUPP;
}

static fileops_t pipeops = {
  .fo_read = pipe_read,
  .fo_write = pipe_write,
  .fo_close = pipe_close,
  .fo_seek = pipe_seek,
  .fo_stat = pipe_stat,
  .fo_ioctl = pipe_ioctl,
};

static file_t *make_pipe_file(pipe_t *pipe, unsigned file_flags) {
  file_t *file = file_alloc();
  file->f_data = pipe;
  file->f_ops = &pipeops;
  file->f_type = FT_PIPE;
  file->f_flags = file_flags;
  file_hold(file);
  return file;
}

int do_pipe2(proc_t *p, int fds[2], int flags) {
  pipe_t *pipe = pipe_alloc();

  file_t *cons_file = make_pipe_file(pipe, FF_READ);
  file_t *prod_file = make_pipe_file(pipe, FF_WRITE);

  int cloexec_to_set = flags & O_CLOEXEC;
  int error;

  if (!(error = fdtab_install_file(p->p_fdtable, cons_file, 0, &fds[0]))) {
    if (!(error = fdtab_install_file(p->p_fdtable, prod_file, 0, &fds[1]))) {
      if (!(error = fd_set_cloexec(p->p_fdtable, fds[0], cloexec_to_set))) {
        if (!(error = fd_set_cloexec(p->p_fdtable, fds[1], cloexec_to_set)))
          goto out;
      }
      fdtab_close_fd(p->p_fdtable, fds[1]);
    }
    fdtab_close_fd(p->p_fdtable, fds[0]);
  }

out:
  file_drop(cons_file);
  file_drop(prod_file);
  return error;
}
