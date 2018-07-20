#define KL_LOG KL_FILE
#include <klog.h>
#include <mutex.h>
#include <condvar.h>
#include <malloc.h>
#include <pool.h>
#include <errno.h>
#include <pipe.h>
#include <stdc.h>
#include <stat.h>
#include <file.h>
#include <filedesc.h>
#include <thread.h>
#include <proc.h>
#include <ringbuf.h>
#include <sysinit.h>

typedef struct pipe_end pipe_end_t;
typedef struct pipe pipe_t;

struct pipe_end {
  mtx_t mtx;          /*!< protects pipe end internals */
  pipe_t *pipe;       /*!< pointer back to pipe structure */
  bool closed;        /*!< true if this end is closed */
  condvar_t nonempty; /*!< used to wait data to appear in the buffer */
  condvar_t nonfull;  /*!< used to wait for free space in the buffer */
  ringbuf_t buf;      /*!< buffer belongs to writer end */
  pipe_end_t *other;  /*!< the other end of the pipe */
};

struct pipe {
  mtx_t mtx;         /*!< protects reference counter */
  int refcnt;        /*!< pipe can be freed when reaches zero */
  pipe_end_t end[2]; /*!< both pipe ends */
};

static MALLOC_DEFINE(M_PIPE, "pipe", 4, 8);
static POOL_DEFINE(P_PIPE, "pipes", sizeof(pipe_t));

static void pipe_end_setup(pipe_end_t *end, pipe_end_t *other) {
  mtx_init(&end->mtx, MTX_DEF);
  cv_init(&end->nonempty, "pipe_end_empty");
  cv_init(&end->nonfull, "pipe_end_full");
  end->buf.data = kmalloc(M_PIPE, PIPE_SIZE, M_ZERO);
  end->buf.size = PIPE_SIZE;
  end->other = other;
}

static pipe_t *pipe_alloc(void) {
  pipe_t *pipe = pool_alloc(P_PIPE, PF_ZERO);
  mtx_init(&pipe->mtx, MTX_DEF);
  pipe_end_t *end0 = &pipe->end[0];
  pipe_end_t *end1 = &pipe->end[1];
  pipe_end_setup(end0, end1);
  pipe_end_setup(end1, end0);
  end0->pipe = pipe;
  end1->pipe = pipe;
  pipe->refcnt = 2;
  return pipe;
}

static void pipe_free(pipe_t *pipe) {
  int refcnt = 0;

  WITH_MTX_LOCK (&pipe->mtx)
    refcnt = --pipe->refcnt;

  if (refcnt == 0) {
    kfree(M_PIPE, pipe->end[0].buf.data);
    kfree(M_PIPE, pipe->end[1].buf.data);
    pool_free(P_PIPE, pipe);
  }
}

static int pipe_read(file_t *f, thread_t *td, uio_t *uio) {
  pipe_end_t *consumer = f->f_data;
  pipe_end_t *producer = consumer->other;

  assert(!consumer->closed);

  int len = uio->uio_resid;

  /* no read atomicity for now! */
  WITH_MTX_LOCK (&producer->mtx) {
    do {
      int res = ringbuf_read(&producer->buf, uio);
      if (res)
        return res;
      /* notify producer that free space is available */
      cv_broadcast(&producer->nonfull);
      /* nothing left to read? */
      if (uio->uio_resid == 0)
        break;
      /* the buffer is empty so wait for some data to be produced */
      cv_wait(&producer->nonempty, &producer->mtx);
    } while (!producer->closed);
  }

  return len - uio->uio_resid;
}

static int pipe_write(file_t *f, thread_t *td, uio_t *uio) {
  pipe_end_t *producer = f->f_data;
  pipe_end_t *consumer = producer->other;

  assert(!producer->closed);

  /* Reading end is closed, no use in sending data there. */
  if (consumer->closed)
    return -ESPIPE;

  int len = uio->uio_resid;

  /* no write atomicity for now! */
  WITH_MTX_LOCK (&producer->mtx) {
    do {
      int res = ringbuf_write(&producer->buf, uio);
      if (res)
        return res;
      /* notify consumer that new data is available */
      cv_broadcast(&producer->nonempty);
      /* nothing left to write? */
      if (uio->uio_resid == 0)
        break;
      /* buffer is full so wait for some data to be consumed */
      cv_wait(&producer->nonfull, &producer->mtx);
    } while (!consumer->closed);
  }

  return len - uio->uio_resid;
}

static int pipe_close(file_t *f, thread_t *td) {
  pipe_end_t *end = f->f_data;

  WITH_MTX_LOCK (&end->mtx) {
    end->closed = true;
    /* Wake up consumers to let them finish their work! */
    cv_broadcast(&end->nonempty);
  }

  pipe_free(end->pipe);

  return 0;
}

static int pipe_stat(file_t *f, thread_t *td, stat_t *sb) {
  return -ENOTSUP;
}

static int pipe_seek(file_t *f, thread_t *td, off_t offset, int whence) {
  return -ENOTSUP;
}

static fileops_t pipeops = {.fo_read = pipe_read,
                            .fo_write = pipe_write,
                            .fo_close = pipe_close,
                            .fo_seek = pipe_seek,
                            .fo_stat = pipe_stat};

static file_t *make_pipe_file(pipe_end_t *end) {
  file_t *file = file_alloc();
  file->f_data = end;
  file->f_ops = &pipeops;
  file->f_type = FT_PIPE;
  file->f_flags = FF_READ | FF_WRITE;
  return file;
}

int do_pipe(thread_t *td, int fds[2]) {
  pipe_t *pipe = pipe_alloc();
  pipe_end_t *consumer = &pipe->end[0];
  pipe_end_t *producer = &pipe->end[1];

  file_t *file0 = make_pipe_file(consumer);
  file_t *file1 = make_pipe_file(producer);

  int error;

  error = fdtab_install_file(td->td_proc->p_fdtable, file0, &fds[0]);
  if (error)
    goto fail;
  error = fdtab_install_file(td->td_proc->p_fdtable, file1, &fds[1]);
  if (error)
    goto fail;
  return 0;

fail:
  pipe_close(file0, td);
  pipe_close(file1, td);
  return error;
}
