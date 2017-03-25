#include <fork.h>
#include <thread.h>
#include <sync.h>
#include <filedesc.h>
#include <sched.h>
#include <stdc.h>

int do_fork(){
  thread_t *td = thread_self();

  /* Cannot fork non-user threads. */
  assert(td->td_uspace);
  
  thread_t* newtd = thread_create(td->td_name, NULL, NULL);
  
  /* Prevent preemption, so that the copied thread won't run while we're copying
     it. */
  critical_enter();

  /* Clone the thread. Since we don't use fork-oriented thread_t layout, we copy
     all fields one-by one for clarity. */

  /* The new thread is already on the all_thread list, has name and tid set. */
  
  newtd->td_state = td->td_state;
  newtd->td_flags = td->td_flags;
  newtd->td_csnest = td->td_csnest - 1; /* Subtract this section we're currently in. */

  /* Copy user context, modify it accordingly. */
  newtd->td_uctx = td->td_uctx;
  newtd->td_uctx.v0 = 0; /* Set return value for child. */
  newtd->td_uctx.pc += 4; /* Advance child PC so that it follows instructions after syscall. */
  newtd->td_uctx_fpu = td->td_uctx_fpu;
  newtd->td_kframe = td->td_kframe; /* Shallow copy, the new thread won't use kframe anyway. */
  newtd->td_kctx = td->td_kctx;
  newtd->td_kctx.pc = (reg_t)user_exc_leave;
  newtd->td_onfault = td->td_onfault;
  
  /* New thread already has its own kstack, but we need to copy the contents. */
  memcpy((char*)newtd->td_kstack_obj->vaddr, (char*)td->td_kstack_obj->vaddr, td->td_kstack_obj->size);
  /* Compute the offset of SP into stack. */
  off_t sp_off = td->td_kctx.sp - td->td_kstack_obj->vaddr;
  newtd->td_kctx.sp = newtd->td_kstack_obj->vaddr + sp_off;

  /* Share userspace memory with parent. */
  newtd->td_uspace = td->td_uspace;

  /* Copy the parent descriptor table. */
  newtd->td_fdtable = fdtab_copy(td->td_fdtable);
  fdtab_ref(newtd->td_fdtable);

  newtd->td_sleepqueue = td->td_sleepqueue;
  newtd->td_wchan = td->td_wchan;
  newtd->td_wmesg = td->td_wmesg;

  newtd->td_prio = td->td_prio;
  newtd->td_slice = td->td_slice;
   
  critical_leave();

  sched_add(newtd);

  return newtd->td_tid;
}
