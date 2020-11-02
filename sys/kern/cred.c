#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/libkern.h>
#include <sys/errno.h>

void cred_fork(proc_t *to, proc_t *from) {
  memcpy(&to->p_cred, &from->p_cred, sizeof(cred_t));
}

int cred_cansignal(proc_t *p, cred_t *cred) {
  assert(mtx_owned(&p->p_lock));

  if (cred->cr_ruid != p->p_cred.cr_ruid &&
      cred->cr_ruid != p->p_cred.cr_suid &&
      cred->cr_euid != p->p_cred.cr_ruid && cred->cr_euid != p->p_cred.cr_suid)
    return EPERM;

  return 0;
}
