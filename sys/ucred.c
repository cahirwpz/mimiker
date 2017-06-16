#include <ucred.h>

int check_groups(gid_t group_id, ucred_t *ucred) {
  for (int i = 0; i < ucred->cr_ngroups; i++) {
    if (group_id == ucred->cr_groups[i])
      return 1;
  }
  return 0;
}
