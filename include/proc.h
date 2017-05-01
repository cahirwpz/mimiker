#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <common.h>
#include <queue.h>
#include <mutex.h>

typedef struct thread thread_t;

typedef struct proc {
  mtx_t p_lock;                   /* Process lock */
  TAILQ_ENTRY(proc) p_all;        /* A link on all processes list */
  TAILQ_HEAD(, thread) p_threads; /* Threads belonging to this process */
  pid_t p_pid;                    /* Process ID */
  struct proc *p_parent;          /* Parent process */
} proc_t;

void proc_init();

proc_t *proc_create();

/* Links a thread with a process, setting td_proc and updating p_threads. */
void proc_add_thread(proc_t *p, thread_t *td);

/* Searches for a process with the given PID, returns NULL if not found. */
proc_t *proc_get_by_pid(pid_t pid);

#endif /* _SYS_PROC_H_ */
