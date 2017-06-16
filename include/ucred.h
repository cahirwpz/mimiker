#ifndef _SYS_UCRED_H_
#define _SYS_UCRED_H_

#include <file.h>

/* access function */
#define F_OK 0      /* test for existence of file */
#define X_OK 0x01   /* test for execute or search permission */
#define W_OK 0x02   /* test for write permission */
#define R_OK 0x04   /* test for read permission */
#define ALL_OK 0x07 /* test all execute, write, read */

typedef struct ucred {
  uid_t cr_uid; /* owner user id */
  gid_t cr_gid;
  gid_t *cr_groups;
  int cr_ngroups;
} ucred_t;

/* Check if user has given group id. */
int check_groups(gid_t group_id, ucred_t *ucred);

#endif /* !_SYS_FILE_H_ */
