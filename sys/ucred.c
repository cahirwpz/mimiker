#include <ucred.h>

bool groupmember(gid_t gid, ucred_t *cred) {
  if (gid == cred->cr_gid)
    return true;
  for (int i = 0; i < cred->cr_ngroups; i++) {
    if (gid == cred->cr_groups[i])
      return true;
  }
  return false;
}
