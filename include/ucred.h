#ifndef _SYS_UCRED_H_
#define _SYS_UCRED_H_

#include <common.h>

typedef struct ucred {
  uid_t cr_uid; /* owner user id */
  gid_t cr_gid;
  gid_t *cr_groups;
  int cr_ngroups;
} ucred_t;

/* Check if user has given group id. */
int check_groups(gid_t group_id, ucred_t *ucred);

#endif /* !_SYS_UCRED_H_ */
