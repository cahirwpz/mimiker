#include "utest.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

TEST_ADD(get_set_uid, 0) {
  uid_t ruid, euid, suid;

  /* check if nothing fail if we put wrong addresses */
  syscall_fail(getresuid((void *)1, (void *)1, (void *)1), EFAULT);

  syscall_ok(getresuid(&ruid, &euid, &suid));

  /* assume we are running tests as root */
  assert(ruid == 0 && euid == 0 && suid == 0);

  /* as root we chan change id to ony other */
  ruid = 1, euid = 2, suid = 3;
  syscall_ok(setresuid(ruid, euid, suid));

  syscall_ok(getresuid(&ruid, &euid, &suid));
  assert(ruid == 1 && euid == 2 && suid == 3);

  /* we can only change to value that is one of real, effective or saved */
  ruid = -1, euid = 3, suid = -1;
  syscall_ok(setresuid(ruid, euid, suid));

  syscall_ok(getresuid(&ruid, &euid, &suid));
  assert(ruid == 1 && euid == 3 && suid == 3);

  /* we cannnot change to value that is not one of real, effective or saved */
  ruid = -1, euid = 2, suid = -1;
  syscall_fail(setresuid(ruid, euid, suid), EPERM);

  syscall_ok(getresuid(&ruid, &euid, &suid));
  assert(ruid == 1 && euid == 3 && suid == 3);

  return 0;
}

TEST_ADD(get_set_gid, 0) {
  gid_t rgid, egid, sgid;

  /* check if nothing fail if we put wrong addresses */
  syscall_fail(getresgid((void *)1, (void *)1, (void *)1), EFAULT);

  syscall_ok(getresgid(&rgid, &egid, &sgid));

  /* assume we are running tests as root */
  assert(rgid == 0 && egid == 0 && sgid == 0);

  /* as root we chan change id to ony other */
  rgid = 1, egid = 2, sgid = 3;
  syscall_ok(setresgid(rgid, egid, sgid));

  syscall_ok(getresgid(&rgid, &egid, &sgid));
  assert(rgid == 1 && egid == 2 && sgid == 3);

  /* dropping privileges */
  syscall_ok(setresuid(1, 1, 1));

  /* we can only change to value that is one of real, effective or saved */
  rgid = -1, egid = 3, sgid = -1;
  syscall_ok(setresgid(rgid, egid, sgid));

  syscall_ok(getresgid(&rgid, &egid, &sgid));
  assert(rgid == 1 && egid == 3 && sgid == 3);

  /* we cannnot change to value that is not one of real, effective or saved */
  rgid = -1, egid = 2, sgid = -1;
  syscall_fail(setresgid(rgid, egid, sgid), EPERM);

  syscall_ok(getresgid(&rgid, &egid, &sgid));
  assert(rgid == 1 && egid == 3 && sgid == 3);

  return 0;
}

TEST_ADD(get_set_groups, 0) {
  gid_t rgrp[NGROUPS_MAX], gidset[NGROUPS_MAX] = {0, 1, 2, 3, 4, 5};
  const int ngroups = 6;
  uid_t euid;

  /* check if we are a root at start */
  syscall_ok(getresuid(NULL, &euid, NULL));
  assert(euid == 0);
  syscall_ok(getgroups(0, NULL));

  /* setting too many groups */
  syscall_fail(setgroups(NGROUPS_MAX + 2, gidset), EINVAL);

  /* setting groups (without fail now) */
  syscall_ok(setgroups(ngroups, gidset));
  assert(getgroups(NGROUPS_MAX, rgrp) == ngroups);
  for (int i = 0; i < ngroups; ++i)
    assert(rgrp[i] == gidset[i]);

  /* first argument is too small */
  syscall_fail(getgroups(ngroups - 1, rgrp), EINVAL);

  /* dropping all supplementary groups */
  syscall_ok(setgroups(0, NULL));
  assert(getgroups(NGROUPS_MAX, rgrp) == 0);

  syscall_fail(setgroups(-ngroups, gidset), EINVAL);
  /* setting for further tests */
  syscall_ok(setgroups(ngroups, gidset));

  /* dropping privileges */
  syscall_ok(setresuid(1, 1, 1));

  /* dropping shouldn't affect supplementary groups */
  assert(getgroups(NGROUPS_MAX, rgrp) == ngroups);
  for (int i = 0; i < ngroups; ++i)
    assert(rgrp[i] == gidset[i]);

  /* we can't change suplementary groups when we are not root user */
  syscall_fail(setgroups(ngroups, gidset), EPERM);

  assert(getgroups(NGROUPS_MAX, rgrp) == ngroups);
  for (int i = 0; i < ngroups; ++i)
    assert(rgrp[i] == gidset[i]);

  /* first argument is too small */
  syscall_fail(getgroups(3, rgrp), EINVAL);
  return 0;
}
