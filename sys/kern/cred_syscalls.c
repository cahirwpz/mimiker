#include <sys/cred.h>
#include <sys/proc.h>

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
  return 0;
}

int do_setresgid(proc_t *p, gid_t rgid, gid_t egid, gid_t sgid) {
  return 0;
}
