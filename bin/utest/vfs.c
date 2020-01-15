#include "utest.h"

#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define TESTDIR "/tmp"

#define assert_ok(expr) assert(expr == 0)
#define assert_fail(expr, err) assert(expr == -1 && errno == err)

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
  // assert_ok(chdir("test2"));
  // assert_ok(chdir("../"));
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
