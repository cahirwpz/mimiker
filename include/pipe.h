#include <mutex.h>
#include <condvar.h>

/* size of pipe buffer */
#define PIPE_SIZE 4096

typedef struct pipe {
  mtx_t pipe_mtx;         /* pipe mutex */
  condvar_t pipe_read_cv; /* condvar for waiting for input while reading */
  pipebuf_t pipe_buf;     /* pipe buffer */
} pipe_t;
