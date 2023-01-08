#include "utest.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Shift used fds by 3 so std{in,out,err} are not affected. */
#undef FD_OFFSET
#define FD_OFFSET 3

#define TESTDIR "/tmp"

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

TEST_ADD(vfs_rw) {
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

TEST_ADD(vfs_trunc) {
  int n;
  void *wrbuf = malloc(4096);
  void *rdbuf = malloc(8196);
  fill_random(wrbuf, 4096);

  assert_open_ok(0, TESTDIR "/file", S_IWUSR | S_IRUSR, O_RDWR | O_CREAT);

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

  syscall_fail(truncate(TESTDIR, 1023), EISDIR);
  return 0;
}

TEST_ADD(vfs_dir) {
  syscall_fail(mkdir("/", 0), EEXIST);
  syscall_ok(mkdir(TESTDIR "/test", 0));
  syscall_fail(mkdir(TESTDIR "/test", 0), EEXIST);
  syscall_fail(mkdir(TESTDIR "//test///", 0), EEXIST);
  syscall_ok(rmdir(TESTDIR "/test"));
  syscall_ok(mkdir(TESTDIR "/test", 0));

  syscall_ok(mkdir(TESTDIR "//test2///", 0));
  syscall_ok(access(TESTDIR "/test2", 0));

  syscall_ok(mkdir(TESTDIR "/test3", 0));
  syscall_ok(mkdir(TESTDIR "/test3/subdir1", 0));
  syscall_ok(mkdir(TESTDIR "/test3/subdir2", 0));
  syscall_ok(mkdir(TESTDIR "/test3/subdir3", 0));
  syscall_fail(mkdir(TESTDIR "/test3/subdir1", 0), EEXIST);
  syscall_ok(access(TESTDIR "/test3/subdir2", 0));

  syscall_fail(mkdir(TESTDIR "/test4/subdir4", 0), ENOENT);

  syscall_ok(rmdir(TESTDIR "/test/"));
  syscall_ok(rmdir(TESTDIR "/test2"));
  syscall_fail(rmdir(TESTDIR "/test3"), ENOTEMPTY);
  syscall_ok(rmdir(TESTDIR "/test3/subdir1"));
  syscall_ok(rmdir(TESTDIR "/test3/subdir2"));
  syscall_ok(rmdir(TESTDIR "/test3/subdir3"));
  syscall_ok(rmdir(TESTDIR "/test3"));
  syscall_fail(rmdir(TESTDIR "/test3"), ENOENT);
  syscall_fail(rmdir(TESTDIR "/test4/subdir4"), ENOENT);

  syscall_fail(mkdir(TESTDIR "/test3/subdir1", 0), ENOENT);

  syscall_fail(mkdir("/", 0), EEXIST);
  syscall_fail(rmdir("/tmp"), EBUSY);

  return 0;
}

TEST_ADD(vfs_relative_dir) {
  syscall_ok(chdir(TESTDIR));
  syscall_ok(mkdir("test", 0));
  syscall_fail(mkdir("test", 0), EEXIST);
  syscall_fail(mkdir("test///", 0), EEXIST);
  syscall_ok(chdir("test"));

  syscall_ok(mkdir("test2///", 0));
  syscall_ok(rmdir("test2"));

  syscall_ok(mkdir("test3", 0));
  syscall_ok(mkdir("test3/subdir1", 0));
  syscall_ok(mkdir("test3/subdir2", 0));
  syscall_ok(mkdir("test3/subdir3", 0));
  syscall_fail(mkdir("test3/subdir1", 0), EEXIST);
  syscall_ok(access("test3/subdir2", 0));

  syscall_ok(rmdir(TESTDIR "/test/test3/subdir1"));
  syscall_ok(rmdir(TESTDIR "/test/test3/subdir2"));
  syscall_ok(rmdir(TESTDIR "/test/test3/subdir3"));
  syscall_ok(rmdir("test3"));

  syscall_ok(chdir(TESTDIR));
  syscall_ok(rmdir("test"));
  syscall_ok(chdir("/"));
  return 0;
}

TEST_ADD(vfs_dot_dot_dir) {
  syscall_ok(chdir(TESTDIR));

  syscall_ok(mkdir("test", 0));
  syscall_ok(chdir("test"));
  syscall_ok(mkdir("test2///", 0));
  syscall_ok(chdir("test2"));

  syscall_ok(chdir(".."));
  syscall_ok(chdir("test2"));

  syscall_ok(chdir("../test2"));
  syscall_ok(chdir("../../"));
  syscall_fail(mkdir("test", 0), EEXIST);

  syscall_ok(chdir("test"));
  syscall_ok(rmdir("../test/test2"));

  syscall_ok(chdir("./.."));
  syscall_ok(rmdir("test"));

  return 0;
}

TEST_ADD(vfs_dot_dir) {
  syscall_fail(mkdir(TESTDIR "/test/.", 0), ENOENT);
  syscall_fail(mkdir("/.", 0), EEXIST);
  syscall_fail(mkdir(TESTDIR "/.", 0), EEXIST);

  return 0;
}

TEST_ADD(vfs_dot_dot_across_fs) {
  syscall_ok(chdir("/../../../../"));
  syscall_fail(mkdir("dev", 0), EEXIST);

  syscall_ok(chdir("dev/../dev/../../../dev/../../dev"));
  syscall_fail(mkdir("../dev", 0), EEXIST);

  syscall_ok(chdir("../"));
  syscall_fail(mkdir("dev", 0), EEXIST);

  return 0;
}

static void test_vfs_symlink_basic(void) {
  char *buff = malloc(1024);
  syscall_ok(symlink("Hello, world!", TESTDIR "/testlink"));

  assert(readlink(TESTDIR "/testlink", buff, 1024) == 13);
  assert(!strcmp("Hello, world!", buff));

  memset(buff, 0, 13);
  assert(readlink(TESTDIR "/testlink", buff, 5) == 5);
  assert(!strcmp("Hello", buff));

  syscall_fail(symlink("Hello, world!", TESTDIR "/testlink"), EEXIST);

  syscall_ok(unlink(TESTDIR "/testlink"));
  free(buff);
}

static void test_vfs_symlink_vnr(void) {
  int n;
  struct stat sb;
  ino_t fileino;

  assert_open_ok(0, TESTDIR "/file", 0, O_RDWR | O_CREAT);
  syscall_ok(stat(TESTDIR "/file", &sb));
  fileino = sb.st_ino;

  /* Absolute symlink */
  syscall_ok(symlink(TESTDIR "/file", TESTDIR "/alink"));
  syscall_ok(stat(TESTDIR "/alink", &sb));
  assert(fileino == sb.st_ino);

  syscall_ok(symlink(TESTDIR "/alink", TESTDIR "/alink2"));
  syscall_ok(stat(TESTDIR "/alink2", &sb));
  assert(fileino == sb.st_ino);

  /* Relative symlink */
  syscall_ok(symlink("file", TESTDIR "/rlink"));
  syscall_ok(stat(TESTDIR "/rlink", &sb));
  assert(fileino == sb.st_ino);

  syscall_ok(symlink("alink2", TESTDIR "/rlink2"));
  syscall_ok(stat(TESTDIR "/rlink2", &sb));
  assert(fileino == sb.st_ino);

  /* Do not follow symlink */
  syscall_ok(lstat(TESTDIR "/alink2", &sb));
  assert(fileino != sb.st_ino);

  unlink(TESTDIR "/alink");
  unlink(TESTDIR "/alink2");
  unlink(TESTDIR "/rlink");
  unlink(TESTDIR "/rlink2");

  /* Symlink to directory */
  syscall_ok(symlink("/tmp", TESTDIR "/dlink"));
  syscall_ok(stat(TESTDIR "/dlink/file", &sb));
  assert(fileino == sb.st_ino);
  unlink(TESTDIR "/dlink");

  /* Looped symlink */
  syscall_ok(symlink(TESTDIR "/slink", TESTDIR "/slink"));
  syscall_fail(stat(TESTDIR "/slink", &sb), ELOOP);
  unlink(TESTDIR "/slink");

  /* Bad symlink */
  syscall_ok(symlink(TESTDIR "/nofile", TESTDIR "/blink"));
  syscall_fail(stat(TESTDIR "/blink", &sb), ENOENT);
  unlink(TESTDIR "/blink");

  unlink(TESTDIR "/file");
}

TEST_ADD(vfs_symlink) {
  test_vfs_symlink_basic();
  test_vfs_symlink_vnr();
  return 0;
}

TEST_ADD(vfs_link) {
  int n;
  struct stat sb;
  ino_t fileino;

  void *wrbuf = malloc(64);
  void *rdbuf = malloc(32);
  fill_random(wrbuf, 64);

  /* Create file and fill it with random data */
  assert_open_ok(0, TESTDIR "/file", S_IWUSR | S_IRUSR, O_RDWR | O_CREAT);
  syscall_ok(stat(TESTDIR "/file", &sb));
  assert(sb.st_nlink == 1);

  fileino = sb.st_ino;

  assert_write_ok(0, wrbuf, 32);

  /* Make a hard link */
  syscall_ok(link(TESTDIR "/file", TESTDIR "/file2"));
  syscall_ok(stat(TESTDIR "/file2", &sb));

  /* Ensure if inode number and link count is proper */
  assert(sb.st_ino == fileino);
  assert(sb.st_nlink == 2);

  /* Ensure if data is the same */
  assert_open_ok(1, TESTDIR "/file2", 0, O_RDWR);
  assert(read(4, rdbuf, 32) == 32);
  assert(!memcmp(wrbuf, rdbuf, 32));

  /* Make another link to the same file*/
  syscall_ok(link(TESTDIR "/file2", TESTDIR "/file3"));
  syscall_ok(stat(TESTDIR "/file3", &sb));

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
  syscall_ok(unlink(TESTDIR "/file2"));

  syscall_ok(stat(TESTDIR "/file", &sb));
  assert(sb.st_nlink == 2);

  syscall_ok(unlink(TESTDIR "/file"));

  syscall_ok(stat(TESTDIR "/file3", &sb));
  assert(sb.st_nlink == 1);

  syscall_ok(unlink(TESTDIR "/file3"));

  syscall_fail(link("/tmp", "/tmp/foo"), EPERM);

  return 0;
}

TEST_ADD(vfs_chmod) {
  struct stat sb;

  assert(open(TESTDIR "/file", O_RDWR | O_CREAT, 0) == 3);
  syscall_ok(stat(TESTDIR "/file", &sb));
  assert((sb.st_mode & ALLPERMS) == 0);

  syscall_ok(chmod(TESTDIR "/file", DEFFILEMODE));
  syscall_ok(stat(TESTDIR "/file", &sb));
  assert((sb.st_mode & ALLPERMS) == DEFFILEMODE);

  mode_t mode = S_IXGRP | S_IWOTH | S_IRUSR | S_ISUID;
  syscall_ok(chmod(TESTDIR "/file", mode));
  syscall_ok(stat(TESTDIR "/file", &sb));
  assert((sb.st_mode & ALLPERMS) == mode);

  mode_t lmode = S_IWUSR | S_IRWXU | S_IRWXO;
  syscall_ok(symlink(TESTDIR "/file", TESTDIR "/link"));
  syscall_ok(lchmod(TESTDIR "/link", lmode));
  syscall_ok(stat(TESTDIR "/link", &sb));
  assert((sb.st_mode & ALLPERMS) == mode);
  syscall_ok(lstat(TESTDIR "/link", &sb));
  assert((sb.st_mode & ALLPERMS) == lmode);

  unlink(TESTDIR "/file");
  unlink(TESTDIR "/link");

  return 0;
}
