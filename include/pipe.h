#ifndef _SYS_PIPE_H_
#define _SYS_PIPE_H_

/* size of pipe buffer */
#define PIPE_SIZE 4096

typedef struct thread thread_t;

int do_pipe(thread_t *td, int fds[2]);

#endif /* !_SYS_PIPE_H_ */
