#include <ucred.h>
#include <malloc.h>

static MALLOC_DEFINE(M_CRED, "cred", 1, 2);

ucred_t *cr_alloc() {
  return kmalloc(M_CRED, sizeof(ucred_t), M_WAITOK | M_ZERO);
}

void cr_free(ucred_t *cred) {
  kfree(M_CRED, cred);
}

bool cr_groupmember(gid_t gid, ucred_t *cred) {
  if (gid == cred->cr_gid)
    return true;
  for (int i = 0; i < cred->cr_ngroups; i++) {
    if (gid == cred->cr_groups[i])
      return true;
  }
  return false;
}
