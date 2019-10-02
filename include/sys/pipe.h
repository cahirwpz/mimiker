#ifndef _SYS_PIPE_H_
#define _SYS_PIPE_H_

#ifdef _KERNEL

/* size of pipe buffer */
#define PIPE_SIZE 4096

typedef struct proc proc_t;

int do_pipe(proc_t *p, int fds[2]);

#endif /* !_KERNEL */

#endif /* !_SYS_PIPE_H_ */
