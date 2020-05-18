#ifndef _SYS_CRED_H_
#define _SYS_CRED_H_

#include <sys/types.h>
#include <stdint.h>

#define NGROUPS_MAX 16

typedef struct cred cred_t;
typedef struct proc proc_t;

/*
 * Kernel view of credencials
 */
struct cred {
  uid_t cr_euid;                /* effective user id */
  uid_t cr_ruid;                /* real user id */
  uid_t cr_suid;                /* saved user id */
  gid_t cr_egid;                /* effective group id */
  gid_t cr_rgid;                /* real grout id */
  gid_t cr_sgid;                /* saved group id */
  uint8_t cr_ngroups;           /* number of groups */
  gid_t cr_groups[NGROUPS_MAX]; /* groups */
};

/* procedures called by syscalls */
int do_getresuid(proc_t *p, uid_t *ruid, uid_t *euid, uid_t *suid);
int do_getresgid(proc_t *p, gid_t *rgid, gid_t *egid, gid_t *sgid);
int do_setresuid(proc_t *p, uid_t ruid, uid_t euid, uid_t suid);
int do_setresgid(proc_t *p, gid_t rgid, gid_t egid, gid_t sgid);

void cred_init_root(cred_t *c);
void cred_copy(cred_t *src, cred_t *dest);
int cred_can_change_uid(cred_t *c, uid_t uid);
int cred_can_change_gid(cred_t *c, gid_t gid);

#endif /* _SYS_CRED_H_ */
