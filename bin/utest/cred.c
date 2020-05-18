#include "utest.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

int test_getresuid(void) {
  uid_t ruid, euid, suid;
  getresuid(&ruid, &euid, &suid);
  printf("real user id: %d\neffective user id: %d\nsaved user id: %d\n", ruid,
         euid, suid);
  return 0;
}

int test_getresgid(void) {
  gid_t rgid, egid, sgid;
  getresgid(&rgid, &egid, &sgid);
  printf("real group id: %d\neffective group id: %d\nsaved group id: %d\n",
         rgid, egid, sgid);

  return 0;
}

int test_setresuid(void) {
  uid_t ruid, euid, suid;
  int error;
  getresuid(&ruid, &euid, &suid);
  printf("0: ruid: %d\teuid: %d\tsuid: %d\n", ruid, euid, suid);

  error = setresuid(1, 2, 3);
  assert(error == 0);
  getresuid(&ruid, &euid, &suid);
  printf("1: ruid: %d\teuid: %d\tsuid: %d\n", ruid, euid, suid);
  //assert(ruid == 1 && euid == 2 && suid == 3);

  error = setresuid(-1, 3, -1);
  getresuid(&ruid, &euid, &suid);
  assert(error == 0);
  printf("2: ruid: %d\teuid: %d\tsuid: %d\n", ruid, euid, suid);
  //assert(ruid == 1 && euid == 3 && suid == 3);

  error = setresuid(3, 3, 3);
  assert(error == 0);
  //assert(ruid == 3 && euid == 3 && suid == 3);
  printf("3: ruid: %d\teuid: %d\tsuid: %d\n", ruid, euid, suid);
  getresuid(&ruid, &euid, &suid);

  error = setresuid(-1, 2, -1);
  assert(error == EPERM);
  printf("4: ruid: %d\teuid: %d\tsuid: %d\n", ruid, euid, suid);
  getresuid(&ruid, &euid, &suid);
  return 0;
}
