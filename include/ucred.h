#ifndef _SYS_UCRED_H_
#define _SYS_UCRED_H_

#include <common.h>

typedef struct ucred {
  uid_t cr_uid;     /* user id */
  gid_t cr_gid;     /* user group id */
  gid_t *cr_groups; /* auxiliary groups */
  int cr_ngroups;
} ucred_t;

#define NOCRED ((ucred_t *)0) /* no credential available */

ucred_t *cr_alloc(void);
void cr_free(ucred_t *cred);
bool cr_groupmember(gid_t gid, ucred_t *cred);

#endif /* !_SYS_UCRED_H_ */
