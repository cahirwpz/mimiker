#include "utest.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static int n;
#define FD_OFFSET 3
#include "utest_fd.h"

#define TESTDIR "/tmp"

#define assert_ok(expr) assert(expr == 0)
#define assert_fail(expr, err) assert(expr == -1 && errno == err)

/* Generate pseudo random data */
static void gen_test_data(size_t n, uint32_t *data) {
  uint32_t a = 1;
  for (size_t i = 0; i < n; i++) {
    a ^= a << 13;
    a ^= a >> 17;
    a ^= a << 5;
    data[i] = a;
  }
}

int test_vfs_rw(void) {
  uint32_t *buff1 = malloc(4096 * sizeof(uint32_t));
  uint32_t *buff2 = malloc(4096 * sizeof(uint32_t));

  gen_test_data(4096, buff1);

  assert_open_ok(0, TESTDIR "/file", 0, O_RDWR | O_CREAT);

  /* Write and read aligned amount of bytes */
  assert_write_ok(0, buff1, 1024 * sizeof(uint32_t));
  assert_lseek_ok(0, 0, SEEK_SET);
  assert(read(3, buff2, 5000) == 4096);
  assert(!memcmp(buff1, buff2, 1024 * sizeof(uint32_t)));
  assert(read(3, buff2, 5000) == 0);
  assert_lseek_ok(0, 0, SEEK_SET);

  /* Write 3000 bytes in two batches and then partially read */
  assert_write_ok(0, buff1, 2000 * sizeof(uint32_t));
  assert_write_ok(0, buff1 + 2000, 1000 * sizeof(uint32_t));
  assert_lseek_ok(0, 0, SEEK_SET);
  assert(read(3, buff2, 1500 * sizeof(uint32_t)) == 1500 * sizeof(uint32_t));
  assert(!memcmp(buff1, buff2, 1500 * sizeof(uint32_t)));
  assert(read(3, buff2 + 1500, 3000 * sizeof(uint32_t)) ==
         1500 * sizeof(uint32_t));
  assert(!memcmp(buff1, buff2, 1500 * sizeof(uint32_t)));
  assert_lseek_ok(0, 0, SEEK_SET);

  /* Write 32KB to test indirect blocks */
  for (int i = 0; i < 2; i++)
    assert_write_ok(0, buff1, 4096 * sizeof(uint32_t));
  assert_lseek_ok(0, 0, SEEK_SET);
  for (int i = 0; i < 2; i++) {
    assert(read(3, buff2, 4096 * sizeof(uint32_t)) == 4096 * sizeof(uint32_t));
    assert(!memcmp(buff1, buff2, 4096 * sizeof(uint32_t)));
  }

  free(buff1);
  free(buff2);

  close(3);
  unlink(TESTDIR "/file");

  return 0;
}

int test_vfs_dir(void) {
  assert_ok(mkdir(TESTDIR "/test", 0));
  assert_fail(mkdir(TESTDIR "/test", 0), EEXIST);
  assert_fail(mkdir(TESTDIR "//test///", 0), EEXIST);
  assert_ok(rmdir(TESTDIR "/test"));
  assert_ok(mkdir(TESTDIR "/test", 0));

  assert_ok(mkdir(TESTDIR "//test2///", 0));
  assert_ok(access(TESTDIR "/test2", 0));

  assert_ok(mkdir(TESTDIR "/test3", 0));
  assert_ok(mkdir(TESTDIR "/test3/subdir1", 0));
  assert_ok(mkdir(TESTDIR "/test3/subdir2", 0));
  assert_ok(mkdir(TESTDIR "/test3/subdir3", 0));
  assert_fail(mkdir(TESTDIR "/test3/subdir1", 0), EEXIST);
  assert_ok(access(TESTDIR "/test3/subdir2", 0));

  assert_fail(mkdir(TESTDIR "/test4/subdir4", 0), ENOENT);

  assert_ok(rmdir(TESTDIR "/test/"));
  assert_ok(rmdir(TESTDIR "/test2"));
  assert_fail(rmdir(TESTDIR "/test3"), ENOTEMPTY);
  assert_ok(rmdir(TESTDIR "/test3/subdir1"));
  assert_ok(rmdir(TESTDIR "/test3/subdir2"));
  assert_ok(rmdir(TESTDIR "/test3/subdir3"));
  assert_ok(rmdir(TESTDIR "/test3"));
  assert_fail(rmdir(TESTDIR "/test3"), ENOENT);
  assert_fail(rmdir(TESTDIR "/test4/subdir4"), ENOENT);

  assert_fail(mkdir(TESTDIR "/test3/subdir1", 0), ENOENT);
  return 0;
}