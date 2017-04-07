#include <fork.h>
#include <thread.h>
#include <sync.h>
#include <filedesc.h>
#include <sched.h>
#include <stdc.h>
#include <vm_map.h>

int do_fork() {
  thread_t *td = thread_self();

  /* Cannot fork non-user threads. */
  assert(td->td_uspace);

  thread_t *newtd = thread_create(td->td_name, NULL, NULL);

  /* Clone the thread. Since we don't use fork-oriented thread_t layout, we copy
     all fields one-by one for clarity. The new thread is already on the
     all_thread list, has name and tid set. */

  newtd->td_state = td->td_state;
  newtd->td_flags = td->td_flags;
  newtd->td_csnest = td->td_csnest;

  /* Copy user context.. */
  newtd->td_uctx = td->td_uctx;
  newtd->td_uctx_fpu = td->td_uctx_fpu;
  exc_frame_set_retval(&newtd->td_uctx, 0);

  /* New thread does not need the exception frame just yet. */
  newtd->td_kframe = NULL;
  newtd->td_onfault = 0;

  /* The new thread already has a new kernel stack allocated. There is no need
     to copy its contents, it will be discarded anyway. We just prepare the
     thread's kernel context to a fresh one so that it will continue execution
     starting from user_exc_leave (which serves as fork_trampoline). */
  ctx_init(newtd, user_exc_leave, NULL);

  /* Clone the entire process memory space. */
  newtd->td_uspace = vm_map_clone(td->td_uspace);

  /* Copy the parent descriptor table. */
  newtd->td_fdtable = fdtab_copy(td->td_fdtable);

  newtd->td_sleepqueue = sleepq_alloc();
  newtd->td_wchan = NULL;
  newtd->td_wmesg = NULL;

  newtd->td_prio = td->td_prio;

  sched_add(newtd);

  return newtd->td_tid;
}
