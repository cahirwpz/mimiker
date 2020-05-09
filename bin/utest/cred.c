#include "utest.h"

#include <unistd.h>
#include <stdio.h>

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
