#ifndef _SYS_CRED_H_
#define _SYS_CRED_H_

#include <sys/types.h>
#include <sys/syslimits.h>

#ifdef _KERNEL

typedef struct proc proc_t;

/*
 * Kernel view of credencials
 */
typedef struct cred {
  uid_t cr_euid;                /* effective user id */
  uid_t cr_ruid;                /* real user id */
  uid_t cr_suid;                /* saved user id */
  gid_t cr_egid;                /* effective group id */
  gid_t cr_rgid;                /* real grout id */
  gid_t cr_sgid;                /* saved group id */
  uint8_t cr_ngroups;           /* number of groups */
  gid_t cr_groups[NGROUPS_MAX]; /* groups */
} cred_t;

/* procedures called by syscalls */
void do_getresuid(proc_t *p, uid_t *ruid, uid_t *euid, uid_t *suid);
void do_getresgid(proc_t *p, gid_t *rgid, gid_t *egid, gid_t *sgid);
int do_setresuid(proc_t *p, uid_t ruid, uid_t euid, uid_t suid);
int do_setresgid(proc_t *p, gid_t rgid, gid_t egid, gid_t sgid);
int do_setgroups(proc_t *p, int ngroups, const gid_t *gidset);

void cred_fork(proc_t *to, proc_t *from);

#endif /* !_KERNEL */

#endif /* _SYS_CRED_H_ */
