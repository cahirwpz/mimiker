#ifndef _SYS_PIPE_H_
#define _SYS_PIPE_H_

#include <mutex.h>
#include <condvar.h>
#include <thread.h>
#include <file.h>
#include <uio.h>
#include <vnode.h>

/* size of pipe buffer */
#define PIPE_SIZE 4096
#define PIPE_EOF 0x1

int do_pipe(thread_t *td, int fds[2]);

#endif /* !_SYS_PIPE_H_ */
