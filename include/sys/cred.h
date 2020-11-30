#ifndef _SYS_CRED_H_
#define _SYS_CRED_H_

#include <sys/types.h>
#include <sys/syslimits.h>
#include <sys/signal.h>
#include <stdbool.h>

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
int do_setuid(proc_t *p, uid_t uid);
int do_seteuid(proc_t *p, uid_t euid);
int do_setreuid(proc_t *p, uid_t ruid, uid_t euid);
int do_setgid(proc_t *p, gid_t gid);
int do_setegid(proc_t *p, gid_t egid);
int do_setregid(proc_t *p, gid_t rgid, gid_t egid);

/* \note Must be called with from::p_lock held. Returns with form::p_lock held.
 * (We do this only while doing fork, so to is new proc not visible for other
 * processes.)
 */
void cred_fork(proc_t *to, proc_t *from);

/* \note Must be called with p::p_lock held */
void cred_exec_setid(proc_t *p, uid_t uid, gid_t gid);

/* \note Must be called with p::p_lock. Returns p::p_lock held. */
int cred_cansignal(proc_t *p, cred_t *cred);

/* Checks whether the current process has permission to send `sig` to `target`
 * process. \note Must be called with all_proc_mtx and target::p_lock. Returns
 * with lock held. */
int proc_cansignal(proc_t *target, signo_t sig);

/* Checks if given gid is in supplementary groups of given credentials set */
bool cred_groupmember(gid_t gid, cred_t *cred);

/* VFS checks */
bool cred_can_chmod(uid_t f_owner, gid_t f_group, cred_t *cred, mode_t mode);
bool cred_can_chown(uid_t f_owner, cred_t *cred, uid_t new_uid, gid_t new_gid);

#endif /* !_KERNEL */

#endif /* _SYS_CRED_H_ */
