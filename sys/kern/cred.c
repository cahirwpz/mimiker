#include <sys/klog.h>
#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/libkern.h>
#include <sys/mutex.h>

cred_t *cred_self(void) {
  return &proc_self()->p_cred;
}

void cred_fork(proc_t *to, proc_t *from) {
  assert(mtx_owned(&from->p_lock));
  memcpy(&to->p_cred, &from->p_cred, sizeof(cred_t));
}

void cred_exec_setid(proc_t *p, uid_t uid, gid_t gid) {
  assert(mtx_owned(&p->p_lock));
  if (uid != (uid_t)-1) {
    p->p_cred.cr_suid = p->p_cred.cr_euid;
    p->p_cred.cr_euid = uid;
  }
  if (gid != (gid_t)-1) {
    p->p_cred.cr_sgid = p->p_cred.cr_egid;
    p->p_cred.cr_egid = gid;
  }
}

bool cred_groupmember(gid_t gid, cred_t *cred) {
  for (int i = 0; i < cred->cr_ngroups; ++i) {
    if (gid == cred->cr_groups[i])
      return true;
  }
  return false;
}
