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

int test_vfs_trunc(void) {
  int n;
  void *wrbuf = malloc(4096);
  void *rdbuf = malloc(8196);
  fill_random(wrbuf, 4096);

  assert_open_ok(0, TESTDIR "/file", 0, O_RDWR | O_CREAT);

  assert_write_ok(0, wrbuf, 4096);
  ftruncate(3, 2048);

  /* The file offset is bigger than size, so read should return 0 bytes. */
  assert(read(3, rdbuf, 6000) == 0);

  assert_lseek_ok(0, 0, SEEK_SET);
  assert(read(3, rdbuf, 6000) == 2048);
  assert(!memcmp(wrbuf, rdbuf, 2048));

  ftruncate(3, 7777);
  assert_lseek_ok(0, 0, SEEK_SET);
  assert(read(3, rdbuf, 10000) == 7777);
  assert(!memcmp(wrbuf, rdbuf, 2048));
  /* Rest of the file should be zeroed. */
  for (int i = 2048; i < 7777; i++)
    assert(((uint8_t *)rdbuf)[i] == 0);

  truncate(TESTDIR "/file", 0);
  assert_lseek_ok(0, 0, SEEK_SET);
  assert(read(3, rdbuf, 2048) == 0);

  free(wrbuf);
  free(rdbuf);
  unlink(TESTDIR "/file");

  assert_fail(truncate(TESTDIR, 1023), EISDIR);
  return 0;
}

int test_vfs_dir(void) {
  assert_fail(mkdir("/", 0), EEXIST);
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

  assert_fail(mkdir("/", 0), EEXIST);
  assert_fail(rmdir("/tmp"), EBUSY);

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

int test_vfs_dot_dot_dir(void) {
  assert_ok(chdir(TESTDIR));

  assert_ok(mkdir("test", 0));
  assert_ok(chdir("test"));
  assert_ok(mkdir("test2///", 0));
  assert_ok(chdir("test2"));

  assert_ok(chdir(".."));
  assert_ok(chdir("test2"));

  assert_ok(chdir("../test2"));
  assert_ok(chdir("../../"));
  assert_fail(mkdir("test", 0), EEXIST);

  assert_ok(chdir("test"));
  assert_ok(rmdir("../test/test2"));

  assert_ok(chdir("./.."));
  assert_ok(rmdir("test"));

  return 0;
}

int test_vfs_dot_dir(void) {
  assert_fail(mkdir(TESTDIR "/test/.", 0), ENOENT);
  assert_fail(mkdir("/.", 0), EEXIST);
  assert_fail(mkdir(TESTDIR "/.", 0), EEXIST);

  return 0;
}

int test_vfs_dot_dot_across_fs(void) {
  assert_ok(chdir("/../../../../"));
  assert_fail(mkdir("dev", 0), EEXIST);

  assert_ok(chdir("dev/../dev/../../../dev/../../dev"));
  assert_fail(mkdir("../dev", 0), EEXIST);

  assert_ok(chdir("../"));
  assert_fail(mkdir("dev", 0), EEXIST);

  return 0;
}

static void test_vfs_symlink_basic(void) {
  char *buff = malloc(1024);
  assert_ok(symlink("Hello, world!", TESTDIR "/testlink"));

  assert(readlink(TESTDIR "/testlink", buff, 1024) == 13);
  assert(!strcmp("Hello, world!", buff));

  memset(buff, 0, 13);
  assert(readlink(TESTDIR "/testlink", buff, 5) == 5);
  assert(!strcmp("Hello", buff));

  assert_fail(symlink("Hello, world!", TESTDIR "/testlink"), EEXIST);

  assert_ok(unlink(TESTDIR "/testlink"));
  free(buff);
}

static void test_vfs_symlink_vnr(void) {
  int n;
  struct stat sb;
  ino_t fileino;

  assert_open_ok(0, TESTDIR "/file", 0, O_RDWR | O_CREAT);
  assert_ok(stat(TESTDIR "/file", &sb));
  fileino = sb.st_ino;

  /* Absolute symlink */
  assert_ok(symlink(TESTDIR "/file", TESTDIR "/alink"));
  assert_ok(stat(TESTDIR "/alink", &sb));
  assert(fileino == sb.st_ino);

  assert_ok(symlink(TESTDIR "/alink", TESTDIR "/alink2"));
  assert_ok(stat(TESTDIR "/alink2", &sb));
  assert(fileino == sb.st_ino);

  /* Relative symlink */
  assert_ok(symlink("file", TESTDIR "/rlink"));
  assert_ok(stat(TESTDIR "/rlink", &sb));
  assert(fileino == sb.st_ino);

  assert_ok(symlink("alink2", TESTDIR "/rlink2"));
  assert_ok(stat(TESTDIR "/rlink2", &sb));
  assert(fileino == sb.st_ino);

  /* Do not follow symlink */
  assert_ok(lstat(TESTDIR "/alink2", &sb));
  assert(fileino != sb.st_ino);

  unlink(TESTDIR "/alink");
  unlink(TESTDIR "/alink2");
  unlink(TESTDIR "/rlink");
  unlink(TESTDIR "/rlink2");

  /* Symlink to directory */
  assert_ok(symlink("/tmp", TESTDIR "/dlink"));
  assert_ok(stat(TESTDIR "/dlink/file", &sb));
  assert(fileino == sb.st_ino);
  unlink(TESTDIR "/dlink");

  /* Looped symlink */
  assert_ok(symlink(TESTDIR "/slink", TESTDIR "/slink"));
  assert_fail(stat(TESTDIR "/slink", &sb), ELOOP);
  unlink(TESTDIR "/slink");

  /* Bad symlink */
  assert_ok(symlink(TESTDIR "/nofile", TESTDIR "/blink"));
  assert_fail(stat(TESTDIR "/blink", &sb), ENOENT);
  unlink(TESTDIR "/blink");

  unlink(TESTDIR "/file");
}

int test_vfs_symlink(void) {
  test_vfs_symlink_basic();
  test_vfs_symlink_vnr();
  return 0;
}

int test_vfs_link(void) {
  int n;
  struct stat sb;
  ino_t fileino;

  void *wrbuf = malloc(64);
  void *rdbuf = malloc(32);
  fill_random(wrbuf, 64);

  /* Create file and fill it with random data */
  assert_open_ok(0, TESTDIR "/file", 0, O_RDWR | O_CREAT);
  assert_ok(stat(TESTDIR "/file", &sb));
  assert(sb.st_nlink == 1);

  fileino = sb.st_ino;

  assert_write_ok(0, wrbuf, 32);

  /* Make a hard link */
  assert_ok(link(TESTDIR "/file", TESTDIR "/file2"));
  assert_ok(stat(TESTDIR "/file2", &sb));

  /* Ensure if inode number and link count is proper */
  assert(sb.st_ino == fileino);
  assert(sb.st_nlink == 2);

  /* Ensure if data is the same */
  assert_open_ok(1, TESTDIR "/file2", 0, O_RDWR);
  assert(read(4, rdbuf, 32) == 32);
  assert(!memcmp(wrbuf, rdbuf, 32));

  /* Make another link to the same file*/
  assert_ok(link(TESTDIR "/file2", TESTDIR "/file3"));
  assert_ok(stat(TESTDIR "/file3", &sb));

  /* Ensure if inode number and link count is proper */
  assert(sb.st_ino == fileino);
  assert(sb.st_nlink == 3);

  assert_open_ok(2, TESTDIR "/file3", 0, O_RDWR);

  /* Make a change to the first file and check for change*/
  assert_lseek_ok(0, 0, SEEK_SET);
  assert_write_ok(0, wrbuf + 32, 32);
  assert(read(5, rdbuf, 32) == 32);
  assert(!memcmp(wrbuf + 32, rdbuf, 32));

  /* Delete second file */
  assert_close_ok(1);
  assert_ok(unlink(TESTDIR "/file2"));

  assert_ok(stat(TESTDIR "/file", &sb));
  assert(sb.st_nlink == 2);

  assert_ok(unlink(TESTDIR "/file"));

  assert_ok(stat(TESTDIR "/file3", &sb));
  assert(sb.st_nlink == 1);

  assert_ok(unlink(TESTDIR "/file3"));

  assert_fail(link("/tmp", "/tmp/foo"), EPERM);

  return 0;
}

int test_vfs_chmod(void) {
  struct stat sb;

  assert(open(TESTDIR "/file", O_RDWR | O_CREAT, 0) == 3);
  assert_ok(stat(TESTDIR "/file", &sb));
  assert((sb.st_mode & ALLPERMS) == 0);

  assert_ok(chmod(TESTDIR "/file", DEFFILEMODE));
  assert_ok(stat(TESTDIR "/file", &sb));
  assert((sb.st_mode & ALLPERMS) == DEFFILEMODE);

  mode_t mode = S_IXGRP | S_IWOTH | S_IRUSR | S_ISUID;
  assert_ok(chmod(TESTDIR "/file", mode));
  assert_ok(stat(TESTDIR "/file", &sb));
  assert((sb.st_mode & ALLPERMS) == mode);

  mode_t lmode = S_IWUSR | S_IRWXU | S_IRWXO;
  assert_ok(symlink(TESTDIR "/file", TESTDIR "/link"));
  assert_ok(lchmod(TESTDIR "/link", lmode));
  assert_ok(stat(TESTDIR "/link", &sb));
  assert((sb.st_mode & ALLPERMS) == mode);
  assert_ok(lstat(TESTDIR "/link", &sb));
  assert((sb.st_mode & ALLPERMS) == lmode);

  unlink(TESTDIR "/file");
  unlink(TESTDIR "/link");

  return 0;
}