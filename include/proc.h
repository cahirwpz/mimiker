#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <common.h>
#include <queue.h>
#include <mutex.h>
#include <condvar.h>
#include <signal.h>
#include <vm_map.h>

/*
 * One structure allocated per process group.
 *
 * List of locks
 * (m)		locked by pg_mtx mtx
 * (e)		locked by proctree_lock sx
 * (c)		const until freeing
 */
struct pgrp {
	LIST_ENTRY(pgrp) pg_hash;	/* (e) Hash chain. */
	LIST_HEAD(, proc) pg_members;	/* (m + e) Pointer to pgrp members. */
	struct sigiolst	pg_sigiolst;	/* (m) List of sigio sources. */
	pid_t		pg_id;		/* (c) Process group id. */
	struct mtx	pg_mtx;		/* Mutex to protect members */
};

typedef struct thread thread_t;
typedef struct proc proc_t;
typedef struct fdtab fdtab_t;
typedef TAILQ_HEAD(, proc) proc_list_t;

typedef enum { PS_NORMAL, PS_DYING, PS_ZOMBIE } proc_state_t;

/*! \brief Process structure
 *
 * Field markings and the corresponding locks:
 *  - a: all_proc_mtx
 *  - p: proc_t::p_lock
 *  - @: read-only access
 *  - ~: always safe to access
 */
struct proc {
  mtx_t p_lock;               /* Process lock */
  TAILQ_ENTRY(proc) p_all;    /* (a) link on all processes list */
  TAILQ_ENTRY(proc) p_zombie; /* (a) link on zombie process list */
  TAILQ_ENTRY(proc) p_child;  /* (a) link on parent's children list */
  thread_t *p_thread;         /* (p) the only thread running in this process */
  pid_t p_pid;                /* (@) Process ID */
  LIST_ENTRY(proc) p_pglist;  /* (g+e) List of processes in pgrp
 				   g - process group mtx
			       	   e - locked by proctree_lock lock
			      */
  struct pgrp *p_pgrp;        /* (c + e) Pointer to process group.
				    c - locked by proc mtx
			      */
  volatile proc_state_t p_state;  /* (p) process state */
  proc_t *p_parent;               /* (a) parent process */
  proc_list_t p_children;         /* (a) child processes, including zombies */
  vm_map_t *p_uspace;             /* (p) process' user space map */
  fdtab_t *p_fdtable;             /* (p) file descriptors table */
  sigaction_t p_sigactions[NSIG]; /* (p) description of signal actions */
  condvar_t p_waitcv;             /* (p) processes waiting for this one */
  int p_exitstatus;               /* (p) exit code to be returned to parent */
  /* program segments */
  vm_segment_t *p_sbrk; /* (p) The entry where brk segment resides in. */
  vaddr_t p_sbrk_end;   /* (p) Current end of brk segment. */
  /* XXX: process resource usage stats */
};

/*! \brief Get a process that currently running thread belongs to. */
proc_t *proc_self(void);

/*! \brief Acquire p::p_lock non-recursive mutex. */
void proc_lock(proc_t *p);

/*! \brief Release p::p_lock non-recursive mutex. */
void proc_unlock(proc_t *p);

DEFINE_CLEANUP_FUNCTION(proc_t *, proc_unlock);

#define WITH_PROC_LOCK(proc)                                                   \
  WITH_STMT(proc_t, proc_lock, CLEANUP_FUNCTION(proc_unlock), proc)

/*! \brief Creates a process and populates it with thread \a td.
 * If parent is not NULL then newly created process becomes its child. */
proc_t *proc_create(thread_t *td, proc_t *parent);

/*! \brief Searches for a process with the given PID and PS_NORMAL state.
 * \returns locked process or NULL if not found */
proc_t *proc_find(pid_t pid);

/*! \brief Called by a processes that wishes to terminate its life.
 * \note Exit status shoud be created using MAKE_STATUS macros from wait.h */
noreturn void proc_exit(int exitstatus);

#endif /* !_SYS_PROC_H_ */
