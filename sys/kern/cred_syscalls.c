#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/libkern.h>
#include <sys/errno.h>

void do_getresuid(proc_t *p, uid_t *ruid, uid_t *euid, uid_t *suid) {
  proc_lock(p);
  cred_t *cr = &p->p_cred;

  if (ruid != NULL)
    *ruid = cr->cr_ruid;

  if (euid != NULL)
    *euid = cr->cr_euid;

  if (suid != NULL)
    *suid = cr->cr_suid;

  proc_unlock(p);
}

void do_getresgid(proc_t *p, gid_t *rgid, gid_t *egid, gid_t *sgid) {
  proc_lock(p);
  cred_t *cr = &p->p_cred;

  if (rgid != NULL)
    *rgid = cr->cr_rgid;

  if (egid != NULL)
    *egid = cr->cr_egid;

  if (sgid != NULL)
    *sgid = cr->cr_sgid;

  proc_unlock(p);
}

/* Process can change any of its user id to given value if:
 *    o is privileged (effective user id == 0)
 *    o given value is equal to any of its user id
 */
static int try_change_uid(cred_t *c, uid_t *uidp, uid_t uid) {
  if (uid == (uid_t)-1)
    return 0;

  if (c->cr_euid != 0 &&
      (uid != c->cr_ruid && uid != c->cr_euid && uid != c->cr_suid))
    return EPERM;

  *uidp = uid;
  return 0;
}

int do_setresuid(proc_t *p, uid_t ruid, uid_t euid, uid_t suid) {
  int error = 0;

  proc_lock(p);

  uid_t new_ruid = p->p_cred.cr_ruid;
  uid_t new_euid = p->p_cred.cr_euid;
  uid_t new_suid = p->p_cred.cr_suid;

  if ((error = try_change_uid(&p->p_cred, &new_ruid, ruid)))
    goto end;

  if ((error = try_change_uid(&p->p_cred, &new_euid, euid)))
    goto end;

  if ((error = try_change_uid(&p->p_cred, &new_suid, suid)))
    goto end;

  p->p_cred.cr_ruid = new_ruid;
  p->p_cred.cr_euid = new_euid;
  p->p_cred.cr_suid = new_suid;

end:
  proc_unlock(p);
  return error;
}

/* Process can change any of its group id to given value if:
 *    o is privileged (effective user id == 0)
 *    o given value is equal to any of its group id
 */
static int cant_change_gid(cred_t *c, gid_t gid) {
  if (c->cr_euid == 0)
    return 0;

  if (gid == c->cr_rgid || gid == c->cr_egid || gid == c->cr_sgid)
    return 0;

  return EPERM;
}

int do_setresgid(proc_t *p, gid_t rgid, gid_t egid, gid_t sgid) {
  proc_lock(p);

  cred_t *oldcred = &p->p_cred;
  cred_t newcred;
  memcpy(&newcred, oldcred, sizeof(cred_t));

  if (rgid != (gid_t)-1) {
    if (cant_change_gid(oldcred, rgid))
      goto fail;

    newcred.cr_rgid = rgid;
  }

  if (egid != (gid_t)-1) {
    if (cant_change_gid(oldcred, egid))
      goto fail;

    newcred.cr_egid = egid;
  }

  if (sgid != (gid_t)-1) {
    if (cant_change_gid(oldcred, sgid))
      goto fail;

    newcred.cr_sgid = sgid;
  }

  memcpy(oldcred, &newcred, sizeof(cred_t));
  proc_unlock(p);
  return 0;

fail:
  proc_unlock(p);
  return EPERM;
}
