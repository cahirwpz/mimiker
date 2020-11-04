#include "utest.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

int test_get_set_uid(void) {
  uid_t ruid, euid, suid;
  int error;

  /* check if nothing fail if we put wrong addresses */
  error = getresuid((void *)1, (void *)1, (void *)1);
  assert(error < 0 && errno == EFAULT);

  error = getresuid(&ruid, &euid, &suid);
  assert(error == 0);

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

int test_get_set_groups(void) {
  gid_t rgrp[NGROUPS_MAX], gidset[NGROUPS_MAX] = {0, 1, 2, 3, 4, 5};
  int r, ngroups = 6;
  uid_t euid;

  /* check if we are a root at start */
  getresuid(NULL, &euid, NULL);
  assert(euid == 0);
  r = getgroups(0, NULL);
  assert(r == 0);

  /* setting too many groups */
  r = setgroups(NGROUPS_MAX + 2, gidset);
  assert(r == -1 && errno == EINVAL);

  /* setting groups (without fail now) */
  r = setgroups(ngroups, gidset);
  assert(r == 0);
  r = getgroups(NGROUPS_MAX, rgrp);
  assert(r == ngroups);
  for (int i = 0; i < r; ++i)
    assert(rgrp[i] == gidset[i]);

  /* first argument is too small */
  r = getgroups(ngroups - 1, rgrp);
  assert(r == -1 && errno == EINVAL);

  /* dropping all supplementary groups */
  r = setgroups(0, NULL);
  assert(r == 0);
  r = getgroups(NGROUPS_MAX, rgrp);
  assert(r == 0);

  r = setgroups(-ngroups, gidset);
  assert(r < 0 && errno == EINVAL);
  /* setting for further tests */
  r = setgroups(ngroups, gidset);
  assert(r == 0);

  /* dropping privileges */
  setresuid(1, 1, 1);

  /* dropping shouldn't affect supplementary groups */
  r = getgroups(NGROUPS_MAX, rgrp);
  assert(r == ngroups);
  for (int i = 0; i < r; ++i)
    assert(rgrp[i] == gidset[i]);

  /* we can't change suplementary groups when we are not root user */
  r = setgroups(ngroups, gidset);
  assert(r == -1 && errno == EPERM);

  r = getgroups(NGROUPS_MAX, rgrp);
  assert(r == ngroups);
  for (int i = 0; i < r; ++i)
    assert(rgrp[i] == gidset[i]);

  /* first argument is too small */
  r = getgroups(3, rgrp);
  assert(r == -1 && errno == EINVAL);
  return 0;
}
