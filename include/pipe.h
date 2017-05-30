#include <mutex.h>
#include <condvar.h>
#include <thread.h>
#include <file.h>
#include <uio.h>
#include <vnode.h>

/* size of pipe buffer */
#define PIPE_SIZE 4096

#define PIPE_EOF 0x1

typedef struct pipebuf {
  size_t cnt;      /* current number of bytes in buffer */
  uint32_t in;     /* in offset (start of write) */
  uint32_t out;    /* out offset (start of read) */
  size_t  size;    /* size of buffer */
  void *data;
} pipebuf_t;

typedef enum pipetype {
  PIPE_READ_END,       /* indicates read end of the pipe */
  PIPE_WRITE_END       /* indicates write end of the pipe */
} pipetype_t;

/* pipe structure, currently Mimiker supports only unidirectional pipes */
typedef struct pipe {
  mtx_t pipe_mtx;         /* pipe mutex */
  condvar_t pipe_read_cv; /* condvar for waiting for input while reading */
  pipebuf_t pipe_buf;     /* pipe buffer at the read endm NULL for write end */
  uint32_t pipe_state;    /* pipe state mask */
  pipetype_t pipe_type;   /* read or write */
  struct pipe *pipe_end;       /* link with another end of pipe, NULL for read end */
} pipe_t;

void pipe_init();
int pipe_read(file_t *f, thread_t *td, uio_t *uio);
int pipe_write(file_t *f, thread_t *td, uio_t *uio);
int pipe_close(file_t *f, thread_t *td);
int pipe_getattr(file_t *f, thread_t *td, vattr_t *va);
