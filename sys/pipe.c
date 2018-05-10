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

typedef struct pipe pipe_t;

struct pipe {
  mtx_t mtx;          /*!< protects pipe internals */
  condvar_t nonempty; /*!< used to wait data to appear in the buffer */
  condvar_t nonfull;  /*!< used to wait for free space in the buffer */
  ringbuf_t buf;      /*!< buffer belongs to writer end */
  bool closed;        /*!< true if this end is closed */
  pipe_t *end;        /*!< the other end of the pipe */
};

MALLOC_DEFINE(M_PIPE, "pipe", 4, 8);

static pool_t P_PIPE;

static int pipe_read(file_t *f, thread_t *td, uio_t *uio) {
  pipe_t *consumer = f->f_data;
  pipe_t *producer = consumer->end;

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
  pipe_t *producer = f->f_data;
  pipe_t *consumer = producer->end;

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
  pipe_t *producer = f->f_data;
  pipe_t *consumer = producer->end;

  WITH_MTX_LOCK (&producer->mtx) {
    producer->closed = true;
    /* Wake up consumers to let them finish their work! */
    cv_broadcast(&producer->nonempty);
  }

  if (producer->closed && consumer->closed) {
    pool_free(P_PIPE, producer);
    pool_free(P_PIPE, consumer);
  }

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

static void pipe_ctor(pipe_t *pipe) {
  mtx_init(&pipe->mtx, MTX_DEF);
  cv_init(&pipe->nonempty, "pipe_empty");
  cv_init(&pipe->nonfull, "pipe_full");
}

static void pipe_dtor(pipe_t *pipe) {
  kfree(M_PIPE, pipe->buf.data);
}

static void pipe_init(void) {
  P_PIPE = pool_create("pipes", sizeof(pipe_t), (pool_ctor_t)pipe_ctor,
                       (pool_dtor_t)pipe_dtor);
}

static pipe_t *make_pipe(file_t *file) {
  pipe_t *pipe = pool_alloc(P_PIPE, 0);

  pipe->buf.data = kmalloc(M_PIPE, PIPE_SIZE, M_ZERO);
  pipe->buf.size = PIPE_SIZE;
  ringbuf_reset(&pipe->buf);

  pipe->closed = false;
  pipe->end = NULL;

  file->f_data = pipe;
  file->f_ops = &pipeops;
  file->f_type = FT_PIPE;
  file->f_flags = FF_READ | FF_WRITE;

  return pipe;
}

/* pipe syscall */
int do_pipe(thread_t *td, int fds[2]) {
  assert(td->td_proc);

  file_t *file0 = file_alloc();
  file_t *file1 = file_alloc();

  pipe_t *consumer = make_pipe(file0);
  pipe_t *producer = make_pipe(file1);

  producer->end = consumer;
  consumer->end = producer;

  int error;

  error = fdtab_install_file(td->td_proc->p_fdtable, file0, &fds[0]);
  if (error)
    goto fail;
  error = fdtab_install_file(td->td_proc->p_fdtable, file1, &fds[1]);
  if (error)
    goto fail;
  return 0;

fail:
  file_destroy(file0);
  file_destroy(file1);
  return error;
}

SYSINIT_ADD(pipe, pipe_init, DEPS("filedesc"));
