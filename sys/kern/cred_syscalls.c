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
 *    o is root user (effective user id == 0)
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

static int change_resuid(cred_t *c, uid_t ruid, uid_t euid, uid_t suid) {
  int error = 0;

  uid_t new_ruid = c->cr_ruid;
  uid_t new_euid = c->cr_euid;
  uid_t new_suid = c->cr_suid;

  if ((error = try_change_uid(c, &new_ruid, ruid)))
    goto end;

  if ((error = try_change_uid(c, &new_euid, euid)))
    goto end;

  if ((error = try_change_uid(c, &new_suid, suid)))
    goto end;

  c->cr_ruid = new_ruid;
  c->cr_euid = new_euid;
  c->cr_suid = new_suid;

end:
  return error;
}

int do_setresuid(proc_t *p, uid_t ruid, uid_t euid, uid_t suid) {
  proc_lock(p);
  int error = change_resuid(&p->p_cred, ruid, euid, suid);
  proc_unlock(p);
  return error;
}

/* Process can change any of its group id to given value if:
 *    o is root user (effective user id == 0)
 *    o given value is equal to any of its group id
 */
static int try_change_gid(cred_t *c, gid_t *gidp, gid_t gid) {
  if (gid == (gid_t)-1)
    return 0;

  if (c->cr_euid != 0 &&
      (gid != c->cr_rgid && gid != c->cr_egid && gid != c->cr_sgid))
    return EPERM;

  *gidp = gid;
  return 0;
}

static int change_resgid(cred_t *c, gid_t rgid, gid_t egid, gid_t sgid) {
  int error = 0;

  gid_t new_rgid = c->cr_rgid;
  gid_t new_egid = c->cr_egid;
  gid_t new_sgid = c->cr_sgid;

  if ((error = try_change_gid(c, &new_rgid, rgid)))
    goto end;

  if ((error = try_change_gid(c, &new_egid, egid)))
    goto end;

  if ((error = try_change_gid(c, &new_sgid, sgid)))
    goto end;

  c->cr_rgid = new_rgid;
  c->cr_egid = new_egid;
  c->cr_sgid = new_sgid;

end:
  return error;
}

int do_setresgid(proc_t *p, gid_t rgid, gid_t egid, gid_t sgid) {
  proc_lock(p);
  int error = change_resgid(&p->p_cred, rgid, egid, sgid);
  proc_unlock(p);
  return error;
}

int do_setgroups(proc_t *p, int ngroups, const gid_t *gidset) {
  /* only root user can change supplementary groups */
  if (p->p_cred.cr_euid != 0)
    return EPERM;

  p->p_cred.cr_ngroups = ngroups;
  memcpy(p->p_cred.cr_groups, gidset, ngroups * sizeof(gid_t));

  return 0;
}

int do_setuid(proc_t *p, uid_t uid) {
  proc_lock(p);

  uid_t ruid = uid, euid = uid, suid = uid;

  if (p->p_cred.cr_euid != 0)
    ruid = suid = -1;

  int error = change_resuid(&p->p_cred, ruid, euid, suid);

  proc_unlock(p);
  return error;
}

int do_seteuid(proc_t *p, uid_t euid) {
  proc_lock(p);
  int error = change_resuid(&p->p_cred, -1, euid, -1);
  proc_unlock(p);
  return error;
}

int do_setreuid(proc_t *p, uid_t ruid, uid_t euid) {
  int error;
  proc_lock(p);

  uid_t cur_euid = p->p_cred.cr_euid;
  uid_t cur_ruid = p->p_cred.cr_ruid;
  uid_t cur_suid = p->p_cred.cr_suid;

  /* if proc is not root it can set ruid only to euid */
  if (cur_euid != 0 && ruid != (uid_t)-1 && ruid != cur_euid) {
    error = EPERM;
    goto end;
  }

  /* if ruid is set
   * or euid is set to value other than current suid or ruid(and of course euid)
   * then suid is set to current euid
   *
   * when we don't set ruid and euid we also don't set suid
   * (the last condition in or)
   */
  if (ruid == (uid_t)-1 && (euid == cur_ruid || euid == cur_euid ||
                            euid == cur_suid || euid == (uid_t)-1))
    cur_euid = -1;

  error = change_resuid(&p->p_cred, ruid, euid, cur_euid);

end:
  proc_unlock(p);
  return error;
}

int do_setgid(proc_t *p, gid_t gid) {
  proc_lock(p);

  gid_t rgid = gid, egid = gid, sgid = gid;

  if (p->p_cred.cr_euid != 0)
    rgid = sgid = -1;

  int error = change_resgid(&p->p_cred, rgid, egid, sgid);

  proc_unlock(p);
  return error;
}

int do_setegid(proc_t *p, gid_t egid) {
  proc_lock(p);
  int error = change_resgid(&p->p_cred, -1, egid, -1);
  proc_unlock(p);
  return error;
}

int do_setregid(proc_t *p, gid_t rgid, gid_t egid) {
  int error;
  proc_lock(p);

  uid_t cur_euid = p->p_cred.cr_euid;
  gid_t cur_egid = p->p_cred.cr_egid;
  gid_t cur_rgid = p->p_cred.cr_rgid;
  gid_t cur_sgid = p->p_cred.cr_sgid;

  /* if proc is not root it can set rgid only to egid */
  if (cur_euid != 0 && rgid != (gid_t)-1 && rgid != cur_egid) {
    error = EPERM;
    goto end;
  }

  /* if rgid is set
   * or egid is set to value other than current sgid or rgid(and of course egid)
   * then sgid is set to current egid
   *
   * when we don't set rgid and egid we also don't set sgid
   * (the last condition in or)
   */
  if (rgid == (gid_t)-1 && (egid == cur_rgid || egid == cur_egid ||
                            egid == cur_sgid || egid == (gid_t)-1))
    cur_egid = -1;

  error = change_resgid(&p->p_cred, rgid, egid, cur_egid);

end:
  proc_unlock(p);
  return error;
}
