#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <common.h>
#include <queue.h>
#include <mutex.h>
#include <vm_map.h>

typedef struct thread thread_t;
typedef struct proc proc_t;
typedef struct fdtab fdtab_t;
typedef TAILQ_HEAD(, thread) thread_list_t;
typedef TAILQ_HEAD(, proc) proc_list_t;

struct proc {
  mtx_t p_lock;            /* Process lock */
  TAILQ_ENTRY(proc) p_all; /* A link on all processes list */
  /* XXX: At the moment we don't support multiple threads in a single process! */
  unsigned p_nthreads;     
  thread_list_t p_threads; /* Threads belonging to this process */
  pid_t p_pid;             /* Process ID */
  proc_t *p_parent;        /* Parent process */
  vm_map_t *p_uspace;      /* process' user space map */
  /* file descriptors table */
  fdtab_t *p_fdtable;
};

void proc_init();

proc_t *proc_create();

/* Links a thread with a process, setting td_proc and updating p_threads. */
void proc_populate(proc_t *p, thread_t *td);

/* Searches for a process with the given PID, returns NULL if not found. */
proc_t *proc_find(pid_t pid);

#endif /* _SYS_PROC_H_ */
