#define KL_LOG KL_PIPE
#include <file.h>
#include <klog.h>
#include <mutex.h>
#include <sysent.h>
#include <malloc.h>
#include <thread.h>

typedef struct pipebuf {
  size_t cnt;      /* current number of bytes in buffer */
  uint32_t in;     /* in offset (start of write) */
  uint32_t out;    /* out offset (start of read) */
  size_t  size;    /* size of buffer */
  void *buffer;
} pipebuf_t;

#include <pipe.h>

MALLOC_DEFINE(M_PIPE, "pipe", 1, 4);

pipe_t *pipe_alloc() {
  pipe_t *pipe = kmalloc(M_PIPE, sizeof(pipe_t), M_ZERO);
  mtx_init(&pipe->pipe_mtx, MTX_DEF);
  cv_init(&pipe->pipe_read_cv, "Condvar for reading");
  return 0;
}

/* Very simplified versions of NetBSD's pipeops */

int pipe_read(file_t *f, thread_t *td, uio_t *uio) {
  assert(td->td_proc);
  pipe_t *pipe = f->f_data;
  pipebuf_t *pipebuf = &pipe->pipe_buf;
  int res = 0;
  int bytes_read = 0;
  
  SCOPED_MTX_LOCK(&pipe->pipe_mtx);

  while (uio->uio_resid > 0) {
    if (pipebuf->cnt > 0) {
      size_t size = pipebuf->size - pipebuf->out;
      if (size > pipebuf->cnt) 
	size = pipebuf->cnt;
      if (size > uio->uio_resid)
	size = uio->uio_resid;

      res = uiomove((char *)pipebuf->buffer+pipebuf->out, size, uio);

      if (res)
	break;

      pipebuf->out += size;
      if (pipebuf->out >= pipebuf->size)
	pipebuf->out = 0;

      pipebuf->cnt -= size;

      if (pipebuf->cnt == 0) {
	pipebuf->in = 0;
	pipebuf->out = 0;
      }
      
      bytes_read += size;
      continue;
    }
    if (bytes_read > 0)
      break;
    cv_wait(&pipe->pipe_read_cv, &pipe->pipe_mtx);
  }
  return res;
}

int pipe_write(file_t *f, thread_t *td, uio_t *uio) {
  return 0;
}

int pipe_close(file_t *f, thread_t *td) {
  return 0;
}

int pipe_getattr(file_t *f, thread_t *td, vattr_t *va) {
  return 0;
}

/* pipe syscall */

int do_pipe(int *fds) {
  pipe_t *pipe = pipe_alloc();
  pipe++;
  return 0;
}


static const fileops_t pipeops = {
  .fo_read = pipe_read,
  .fo_write = pipe_write,
  .fo_close = pipe_close,
  .fo_getattr = pipe_getattr
};
