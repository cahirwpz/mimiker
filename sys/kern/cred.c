#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/mutex.h>

void cred_fork(proc_t *to, proc_t *from) {
  assert(mtx_owned(&from->p_lock));
  memcpy(&to->p_cred, &from->p_cred, sizeof(cred_t));
}

void cred_copy(cred_t *cr, proc_t *p) {
  assert(mtx_owned(&p->p_lock));
  memcpy(cr, &p->p_cred, sizeof(cred_t));
}

int cred_cansignal(proc_t *target, cred_t *cred) {
  assert(mtx_owned(&target->p_lock));

  if (cred->cr_ruid != target->p_cred.cr_ruid &&
      cred->cr_ruid != target->p_cred.cr_suid &&
      cred->cr_euid != target->p_cred.cr_ruid &&
      cred->cr_euid != target->p_cred.cr_suid)
    return EPERM;

  return 0;
}
