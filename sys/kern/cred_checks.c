#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/stat.h>

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

bool cred_can_chmod(uid_t f_owner, gid_t f_group, cred_t *cred, mode_t mode) {
  /* root can chmod */
  if (cred->cr_euid == 0)
    return true;

  /* owner of file can chmod */
  if (f_owner != cred->cr_euid)
    return false;

  /* can't set S_ISGID if file group is neither our egid nor in supplementary
   * group */
  if ((mode & S_ISGID) != 0 && f_group != cred->cr_egid &&
      !cred_groupmember(f_group, cred))
    return false;

  return true;
}

bool cred_can_chown(uid_t f_owner, cred_t *cred, uid_t new_uid, gid_t new_gid) {
  /* root can chown */
  if (cred->cr_euid == 0)
    return true;

  /* only root can change owner of file */
  if (new_uid != (uid_t)-1)
    return false;

  /* owner can change group */
  if (f_owner != cred->cr_euid)
    return false;

  /* only to group which is in his supplementary groups */
  return cred_groupmember(new_gid, cred);
}
