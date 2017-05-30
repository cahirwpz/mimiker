#define KL_LOG KL_PIPE
#include <file.h>
#include <klog.h>
#include <mutex.h>
#include <sysent.h>
#include <malloc.h>
#include <thread.h>
#include <pool.h>
#include <errno.h>
#include <pipe.h>
#include <stdc.h>

MALLOC_DEFINE(M_PIPE, "pipe", 1, 4);

static pool_t pipe_cache;

static void reset_values(pipe_t *pipe) {
  pipe->pipe_buf.cnt = 0;
  pipe->pipe_buf.in = 0;
  pipe->pipe_buf.out = 0;
  pipe->pipe_state = 0;
}

static void get_pipe(pipe_t **dest) {
  *dest = pool_alloc(&pipe_cache, 0);
  reset_values(*dest);
}

void pipe_ctor(void *ptr) {
  pipe_t *pipe = ptr;
  mtx_init(&pipe->pipe_mtx, MTX_DEF);
  cv_init(&pipe->pipe_read_cv, "pipe_read_cv");
  pipe->pipe_buf.data = kmalloc(M_PIPE, PIPE_SIZE, M_ZERO); /* TODO: improve pooled allocator to be able to contain page-sized elements */
  pipe->pipe_buf.size = PIPE_SIZE;
  reset_values(pipe);
}

void pipe_dtor(void *ptr) {
  pipe_t *pipe = ptr;
  kfree(M_PIPE, pipe->pipe_buf.data);
}

void pipe_init() {
  pool_init(&pipe_cache, sizeof(pipe_t), pipe_ctor, pipe_dtor);
}

/* Very simplified versions of NetBSD's pipeops */

int pipe_read(file_t *f, thread_t *td, uio_t *uio) {
  assert(td->td_proc);
  pipe_t *pipe = f->f_data;
  if (pipe->pipe_type != PIPE_READ_END)
    return EINVAL;
  pipebuf_t *pipebuf = &pipe->pipe_buf;
  int res = 0;
  int bytes_read = 0;
  
  SCOPED_MTX_LOCK(&pipe->pipe_mtx);

  while (uio->uio_resid > 0) {
    if (pipebuf->cnt > 0) {
      /* find number of bytes to feed to uiomove */
      size_t size = pipebuf->size - pipebuf->out;
      if (size > pipebuf->cnt) 
	size = pipebuf->cnt;
      if (size > uio->uio_resid)
	size = uio->uio_resid;

      res = uiomove((char *)pipebuf->data+pipebuf->out, size, uio);

      if (res)
	break;

      /* moving out offset by size bytes */
      pipebuf->out += size;
      if (pipebuf->out >= pipebuf->size)
	pipebuf->out = 0;

      pipebuf->cnt -= size;

      /* NetBSD sources say that setting offsets to 0 increases cache hit rate */
      if (pipebuf->cnt == 0) {
	pipebuf->in = 0;
	pipebuf->out = 0;
      }
      
      bytes_read += size;
      continue;
    }
    /* widowed pipe, jump to return (should return 0) */
    if (pipe->pipe_state & PIPE_EOF)
      break;
    /* read data, so time to quit */
    if (bytes_read > 0)
      break;
    /* none of the previous conditions was met so waiting for input */
    cv_wait(&pipe->pipe_read_cv, &pipe->pipe_mtx);
  }
  return res;
}

int pipe_write(file_t *f, thread_t *td, uio_t *uio) {
  assert(td->td_proc);
  pipe_t *wpipe = f->f_data;
  if (wpipe->pipe_type != PIPE_WRITE_END)
    return EINVAL;
  pipe_t *rpipe = wpipe->pipe_end;
  pipebuf_t *pipebuf = &rpipe->pipe_buf;
  int res = 0;

  SCOPED_MTX_LOCK(&wpipe->pipe_mtx);

  while (uio->uio_resid > 0) {
    if (uio->uio_resid > PIPE_SIZE) {
      res = ENOSPC;
      break;
    }
    /* calculate free space in buffer */
    int free_space = pipebuf->size - pipebuf->cnt;
    /* our writes must be atomic */
    if (free_space >= uio->uio_resid) {
      int total_size = uio->uio_resid;
      int move_size = pipebuf->size - pipebuf->in;
      if (move_size > total_size)
	move_size = total_size;

      res = uiomove((char*)pipebuf->data+pipebuf->in, move_size, uio);
      if (res == 0 && move_size < total_size) {
	assert(pipebuf->in + move_size == pipebuf->size);
	res = uiomove((char*)pipebuf->data, total_size - move_size, uio);
      }
      if (res)
	break;
      pipebuf->in += total_size;
      if (pipebuf->in >= pipebuf->size) {
	assert(pipebuf->in == total_size - move_size + pipebuf->size);
	pipebuf->in = total_size - move_size;
      }
      pipebuf->cnt += total_size;
      assert(pipebuf->cnt <= pipebuf->size);
    } else {
      if (rpipe->pipe_state & PIPE_EOF) {
	res = EPIPE;
	break;
      }
      cv_broadcast(&rpipe->pipe_read_cv);
    }
  }

  /* notify read end about successful write */
  if (pipebuf->cnt > 0)
    cv_broadcast(&rpipe->pipe_read_cv);
  
  return res;
}

int pipe_close(file_t *f, thread_t *td) {
  assert(td->td_proc);
  pipe_t *pipe = f->f_data;

  mtx_lock(&pipe->pipe_mtx);
  
  pipe->pipe_state |= PIPE_EOF;
  if (pipe->pipe_type == PIPE_WRITE_END) {
    pipe->pipe_end->pipe_state |= PIPE_EOF;
    cv_broadcast(&pipe->pipe_end->pipe_read_cv);
    pipe->pipe_end = NULL;
  }

  mtx_unlock(&pipe->pipe_mtx);

  pool_free(&pipe_cache, pipe);

  return 0;
}

int pipe_getattr(file_t *f, thread_t *td, vattr_t *va) {
  pipe_t *pipe = f->f_data;

  SCOPED_MTX_LOCK(&pipe->pipe_mtx);

  memset(va, 0, sizeof(vattr_t));
  va->st_size = pipe->pipe_buf.cnt;
  return 0;
}

/* pipe syscall */

int do_pipe(thread_t *td, int *fds) {
  pipe_t *pipe;
  get_pipe(&pipe);
  return 0;
}


static const fileops_t pipeops = {
  .fo_read = pipe_read,
  .fo_write = pipe_write,
  .fo_close = pipe_close,
  .fo_getattr = pipe_getattr
};
