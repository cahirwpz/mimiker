#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/signal.h>
#include <sys/vm_map.h>

typedef struct thread thread_t;
typedef struct proc proc_t;
typedef struct pgrp pgrp_t;
typedef struct fdtab fdtab_t;
typedef struct vnode vnode_t;
typedef TAILQ_HEAD(, proc) proc_list_t;
typedef TAILQ_HEAD(, pgrp) pgrp_list_t;

extern mtx_t *all_proc_mtx;

/*! \brief Structure allocated per process group.
 *
 * Field markings and the corresponding locks:
 *  (a) all_proc_mtx
 *  (@) pgrp::pg_lock
 *  (!) read-only access, do not modify!
 */
typedef struct pgrp {
  mtx_t pg_lock;                 /* Process group mutex */
  TAILQ_ENTRY(pgrp) pg_link;     /* (a) link on chain of process groups */
  TAILQ_HEAD(, proc) pg_members; /* (@) members of process group */
  pgid_t pg_id;                  /* (!) process group id */
} pgrp_t;

typedef enum { PS_NORMAL, PS_STOPPED, PS_DYING, PS_ZOMBIE } proc_state_t;

/* Process flags */

/* Set on stopping, cleared when continued or reported by wait4. */
#define PFA_STOPPED 0x1
/* Set when continued, cleared when stopped or reported by wait4. */
#define PFA_CONTINUED 0x2

/*! \brief Process structure
 *
 * Field markings and the corresponding locks:
 *  (a) all_proc_mtx
 *  (@) proc_t::p_lock
 *  (!) read-only access, do not modify!
 *  (~) always safe to access
 *  ($) use only from the same process/thread
 *  (*) safe to dereference from owner process
 */
struct proc {
  mtx_t p_lock;               /* Process lock */
  TAILQ_ENTRY(proc) p_all;    /* (a) link on all processes list */
  TAILQ_ENTRY(proc) p_zombie; /* (a) link on zombie process list */
  TAILQ_ENTRY(proc) p_child;  /* (a) link on parent's children list */
  thread_t *p_thread;         /* (@) the only thread running in this process */
  pid_t p_pid;                /* (!) Process ID */
  char *p_elfpath;            /* (!) path of loaded elf file */
  TAILQ_ENTRY(proc) p_pglist; /* (pgrp::pg_lock) link on pg_members list */
  pgrp_t *p_pgrp;             /* (a,*) process group */
  volatile proc_state_t p_state;  /* (@) process state */
  proc_t *p_parent;               /* (a) parent process */
  proc_list_t p_children;         /* (a) child processes, including zombies */
  vm_map_t *p_uspace;             /* ($) process' user space map */
  fdtab_t *p_fdtable;             /* ($) file descriptors table */
  sigaction_t p_sigactions[NSIG]; /* (@) description of signal actions */
  condvar_t p_waitcv;             /* (a) processes waiting for this one */
  int p_exitstatus;               /* (@) exit code to be returned to parent */
  volatile uint32_t p_aflags;     /* (a) PFA_* flags */
  vnode_t *p_cwd;                 /* ($) current working directory */
  mode_t p_cmask;                 /* ($) mask for file creation */
  /* program segments */
  vm_segment_t *p_sbrk; /* ($) The entry where brk segment resides in. */
  vaddr_t p_sbrk_end;   /* ($) Current end of brk segment. */
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
 * If parent is not NULL then newly created process becomes its child.
 * Created process should be added using proc_add */
proc_t *proc_create(thread_t *td, proc_t *parent);

/*! \brief Adds created process to global data structures. */
void proc_add(proc_t *p);

/*! \brief Searches for a process with the given PID and PS_NORMAL state.
 * \returns locked process or NULL if not found */
proc_t *proc_find(pid_t pid);

/*! \brief Sends signal to process group or process.
 * (pid > 0) sends signal to the process with the ID specified by pid.
 * (pid = 0) sends signal to processes in process group of the calling process.
 * (pid <-1) sends signal to processes in process group with ID equal (-pid). */
int proc_sendsig(pid_t pid, signo_t sig);

/*! \brief Gets process group ID of the process specified by pid.
 * If pid equals zero then use process group ID of the calling process. */
int proc_getpgid(pid_t pid, pgid_t *pgidp);

/*! \brief Called by a processes that wishes to terminate its life.
 * \note Exit status shoud be created using MAKE_STATUS macros from wait.h */
__noreturn void proc_exit(int exitstatus);

/*! \brief Moves process p to the process group with ID specified by pgid.
 * If such process group does not exist then it creates one. */
int pgrp_enter(proc_t *p, pgid_t pgid);

int do_fork(pid_t *cldpidp);

static inline bool proc_is_alive(proc_t *p) {
  return (p->p_state == PS_NORMAL || p->p_state == PS_STOPPED);
}

#endif /* !_SYS_PROC_H_ */
