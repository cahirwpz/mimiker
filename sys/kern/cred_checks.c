#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/stat.h>
#include <sys/vnode.h>

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
  assert(mtx_owned(&all_proc_mtx));
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

bool cred_can_setlogin(cred_t *cred) {
  /* Only root can setlogin(). */
  return cred->cr_euid == 0;
}

int cred_can_access(vattr_t *va, cred_t *cred, accmode_t mode) {
  accmode_t granted = 0;

  if (cred->cr_euid == 0) {
    granted |= VREAD | VWRITE | VADMIN;
    /* root has exec permission when:
     *   - file is directory
     *   - at least one exec bit is se
     */
    if (S_ISDIR(va->va_mode) || va->va_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
      granted |= VEXEC;

    goto check;
  }

  if (cred->cr_euid == va->va_uid) {
    granted |= VADMIN;
    if (va->va_mode & S_IRUSR)
      granted |= VREAD;
    if (va->va_mode & S_IWUSR)
      granted |= VWRITE;
    if (va->va_mode & S_IXUSR)
      granted |= VEXEC;
    goto check;
  }

  if (cred_groupmember(va->va_gid, cred)) {
    if (va->va_mode & S_IRGRP)
      granted |= VREAD;
    if (va->va_mode & S_IWGRP)
      granted |= VWRITE;
    if (va->va_mode & S_IXGRP)
      granted |= VEXEC;
    goto check;
  }

  if (va->va_mode & S_IROTH)
    granted |= VREAD;
  if (va->va_mode & S_IWOTH)
    granted |= VWRITE;
  if (va->va_mode & S_IXOTH)
    granted |= VEXEC;

check:
  return (mode & granted) == mode ? 0 : EACCES;
}

int cred_can_utime(vnode_t *vn, uid_t f_owner, cred_t *cred, int vaflags) {
  /* owner and root can change times */
  if (f_owner == cred->cr_euid || cred->cr_euid == 0)
    return 0;

  /* any other user can change times only to current time... */
  if ((vaflags & VA_UTIMES_NULL) == 0)
    return EPERM;

  /* and only when he has write permission */
  return VOP_ACCESS(vn, VWRITE, cred);
}
