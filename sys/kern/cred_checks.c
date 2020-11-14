#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/mutex.h>

int cred_cansignal(proc_t *target, cred_t *cred) {
  assert(mtx_owned(&target->p_lock));

  if (cred->cr_euid == 0)
    return 0;

  if (cred->cr_ruid != target->p_cred.cr_ruid &&
      cred->cr_ruid != target->p_cred.cr_suid &&
      cred->cr_euid != target->p_cred.cr_ruid &&
      cred->cr_euid != target->p_cred.cr_suid)
    return EPERM;

  return 0;
}

int proc_cansignal(proc_t *target, signo_t sig) {
  assert(mtx_owned(all_proc_mtx));
  assert(mtx_owned(&target->p_lock));

  proc_t *p = proc_self();

  if (target == p)
    return 0;

  /* process can send SIGCONT to every process in the same session */
  if (sig == SIGCONT && p->p_pgrp->pg_session == target->p_pgrp->pg_session)
    return 0;

  return cred_cansignal(target, &p->p_cred);
}
