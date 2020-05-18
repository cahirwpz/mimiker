#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/errno.h>

int do_getresuid(proc_t *p, uid_t *ruid, uid_t *euid, uid_t *suid) {
  cred_t cr = p->p_cred;
  if (ruid != NULL)
    *ruid = cr.cr_ruid;
  if (euid != NULL)
    *euid = cr.cr_euid;
  if (suid != NULL)
    *suid = cr.cr_suid;
  return 0;
}

int do_getresgid(proc_t *p, gid_t *rgid, gid_t *egid, gid_t *sgid) {
  cred_t cr = p->p_cred;
  if (rgid != NULL)
    *rgid = cr.cr_rgid;
  if (egid != NULL)
    *egid = cr.cr_egid;
  if (sgid != NULL)
    *sgid = cr.cr_sgid;
  return 0;
}

int do_setresuid(proc_t *p, uid_t ruid, uid_t euid, uid_t suid) {
  proc_lock(p);

  cred_t *oldcred = &p->p_cred;
  cred_t newcred; /* XXX: maybe not on stack */
  cred_copy(&newcred, oldcred);

  if (cred_can_change_uid(oldcred, ruid) == 0)
    newcred.cr_ruid = ruid;
  else
    goto fail;
  
  if (cred_can_change_uid(oldcred, euid) == 0)
    newcred.cr_euid = euid;
  else
    goto fail;

  if (cred_can_change_uid(oldcred, suid) == 0)
    newcred.cr_suid = suid;
  else
    goto fail;

  cred_copy(oldcred, &newcred);
  proc_unlock(p);
  return 0;

fail:
  proc_unlock(p);
  return EPERM;
}

int do_setresgid(proc_t *p, gid_t rgid, gid_t egid, gid_t sgid) {
  proc_lock(p);

  cred_t *oldcred = &p->p_cred;
  cred_t newcred; /* XXX: maybe not on stack */
  cred_copy(&newcred, oldcred);

  if (cred_can_change_gid(oldcred, rgid) == 0)
    newcred.cr_rgid = rgid;
  else
    goto fail;
  
  if (cred_can_change_gid(oldcred, egid) == 0)
    newcred.cr_egid = egid;
  else
    goto fail;

  if (cred_can_change_gid(oldcred, sgid) == 0)
    newcred.cr_sgid = sgid;
  else
    goto fail;

  cred_copy(oldcred, &newcred);
  proc_unlock(p);
  return 0;

fail:
  proc_unlock(p);
  return EPERM;
}
