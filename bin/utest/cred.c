#include "utest.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

int test_get_set_uid(void) {
  uid_t ruid, euid, suid;
  int error;
  getresuid(&ruid, &euid, &suid);

  /* assume we are running tests as root */
  assert(ruid == 0 && euid == 0 && suid == 0);

  /* as root we chan change id to ony other */
  ruid = 1, euid = 2, suid = 3;
  error = setresuid(ruid, euid, suid);
  assert(error == 0);

  getresuid(&ruid, &euid, &suid);
  assert(ruid == 1 && euid == 2 && suid == 3);

  /* we can only change to value that is one of real, effective or saved */
  ruid = -1, euid = 3, suid = -1;
  error = setresuid(ruid, euid, suid);
  assert(error == 0);

  getresuid(&ruid, &euid, &suid);
  assert(ruid == 1 && euid == 3 && suid == 3);

  /* we cannnot change to value that is not one of real, effective or saved */
  ruid = -1, euid = 2, suid = -1;
  error = setresuid(ruid, euid, suid);
  assert(error < 0 && errno == EPERM);

  getresuid(&ruid, &euid, &suid);
  assert(ruid == 1 && euid == 3 && suid == 3);

  return 0;
}

int test_get_set_gid(void) {
  gid_t rgid, egid, sgid;
  int error;

  /* check if nothing fail if we put wrong addresses */
  error = getresgid((void *)1, (void *)1, (void *)1);
  assert(error < 0 && errno == EFAULT);

  error = getresgid(&rgid, &egid, &sgid);
  assert(error == 0);

  /* assume we are running tests as root */
  assert(rgid == 0 && egid == 0 && sgid == 0);

  /* as root we chan change id to ony other */
  rgid = 1, egid = 2, sgid = 3;
  error = setresgid(rgid, egid, sgid);
  assert(error == 0);

  getresgid(&rgid, &egid, &sgid);
  assert(rgid == 1 && egid == 2 && sgid == 3);

  /* dropping privileges */
  setresuid(1, 1, 1);

  /* we can only change to value that is one of real, effective or saved */
  rgid = -1, egid = 3, sgid = -1;
  error = setresgid(rgid, egid, sgid);
  assert(error == 0);

  getresgid(&rgid, &egid, &sgid);
  assert(rgid == 1 && egid == 3 && sgid == 3);

  /* we cannnot change to value that is not one of real, effective or saved */
  rgid = -1, egid = 2, sgid = -1;
  error = setresgid(rgid, egid, sgid);
  assert(error < 0 && errno == EPERM);

  getresgid(&rgid, &egid, &sgid);
  assert(rgid == 1 && egid == 3 && sgid == 3);

  return 0;
}
