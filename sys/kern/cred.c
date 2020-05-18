#include <sys/cred.h>
#include <sys/libkern.h>
#include <sys/errno.h>

void cred_init_root(cred_t *c) {
  c->cr_ruid = 0;
  c->cr_euid = 0;
  c->cr_suid = 0;
  c->cr_rgid = 0;
  c->cr_egid = 0;
  c->cr_sgid = 0;
  c->cr_ngroups = 0;
}

void cred_copy(cred_t *src, cred_t *dest) {
  memcpy(dest, src, sizeof(cred_t));
}

/* Process can change any of its user id to given value if:
 *    o is privileged (effective user id == 0)
 *    o given value is equal to any of its user id
 */
int cred_can_change_uid(cred_t *c, uid_t uid) {
  if (c->cr_euid == 0)
    return 0;

  if (uid == c->cr_ruid || uid == c->cr_euid || uid == c->cr_suid)
    return 0;

  return EPERM;
}

/* Process can change any of its group id to given value if:
 *    o is privileged (effective user id == 0)
 *    o given value is equal to any of its group id
 */
int cred_can_change_gid(cred_t *c, gid_t gid) {
  if (c->cr_euid == 0)
    return 0;

  if (gid == c->cr_rgid || gid == c->cr_egid || gid == c->cr_sgid)
    return 0;

  return EPERM;
}
