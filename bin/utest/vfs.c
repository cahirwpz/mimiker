#include "utest.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FD_OFFSET 3
#include "utest_fd.h"

#define TESTDIR "/tmp"

#define assert_ok(expr) assert(expr == 0)
#define assert_fail(expr, err) assert(expr == -1 && errno == err)

/* Generate pseudo random data */
static void fill_random(uint32_t *data, size_t n) {
  uint32_t a = 1;
  for (size_t i = 0; i < n / sizeof(uint32_t); i++) {
    a ^= a << 13;
    a ^= a >> 17;
    a ^= a << 5;
    data[i] = a;
  }
}

int test_vfs_rw(void) {
  int n;

  void *wrbuf = malloc(16384);
  void *rdbuf = malloc(16384);

  fill_random(wrbuf, 16384);

  assert_open_ok(0, TESTDIR "/file", 0, O_RDWR | O_CREAT);

  /* Write and read aligned amount of bytes */
  assert_write_ok(0, wrbuf, 4096);
  assert_lseek_ok(0, 0, SEEK_SET);
  assert(read(3, rdbuf, 5000) == 4096);
  assert(!memcmp(wrbuf, rdbuf, 4096));
  assert(read(3, rdbuf, 5000) == 0);
  assert_lseek_ok(0, 0, SEEK_SET);

  /* Write 12000 bytes in two batches and then partially read */
  assert_write_ok(0, wrbuf, 8000);
  assert_write_ok(0, wrbuf + 8000, 4000);
  assert_lseek_ok(0, 0, SEEK_SET);
  assert(read(3, rdbuf, 6000) == 6000);
  assert(!memcmp(wrbuf, rdbuf, 6000));
  assert(read(3, rdbuf + 6000, 12000) == 6000);
  assert(!memcmp(wrbuf, rdbuf, 6000));
  assert_lseek_ok(0, 0, SEEK_SET);

  /* Write 32KiB to test indirect blocks */
  for (int i = 0; i < 2; i++)
    assert_write_ok(0, wrbuf, 16384);
  assert_lseek_ok(0, 0, SEEK_SET);
  for (int i = 0; i < 2; i++) {
    assert(read(3, rdbuf, 16384) == 16384);
    assert(!memcmp(wrbuf, rdbuf, 16384));
  }

  free(wrbuf);
  free(rdbuf);

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

int test_vfs_relative_dir(void) {
  assert_ok(chdir(TESTDIR));
  assert_ok(mkdir("test", 0));
  assert_fail(mkdir("test", 0), EEXIST);
  assert_fail(mkdir("test///", 0), EEXIST);
  assert_ok(chdir("test"));

  assert_ok(mkdir("test2///", 0));
  assert_ok(rmdir("test2"));

  assert_ok(mkdir("test3", 0));
  assert_ok(mkdir("test3/subdir1", 0));
  assert_ok(mkdir("test3/subdir2", 0));
  assert_ok(mkdir("test3/subdir3", 0));
  assert_fail(mkdir("test3/subdir1", 0), EEXIST);
  assert_ok(access("test3/subdir2", 0));

  assert_ok(rmdir(TESTDIR "/test/test3/subdir1"));
  assert_ok(rmdir(TESTDIR "/test/test3/subdir2"));
  assert_ok(rmdir(TESTDIR "/test/test3/subdir3"));
  assert_ok(rmdir("test3"));

  assert_ok(chdir(TESTDIR));
  assert_ok(rmdir("test"));
  assert_ok(chdir("/"));
  return 0;
}
