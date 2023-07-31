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
#define TESTPATH(x) TESTDIR "/" x

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

TEST_ADD(vfs_rw, 0) {
  int n;

  void *wrbuf = malloc(16384);
  void *rdbuf = malloc(16384);

  fill_random(wrbuf, 16384);

  assert_open_ok(0, TESTPATH("file"), 0, O_RDWR | O_CREAT);

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

  xclose(3);
  xunlink(TESTPATH("file"));

  return 0;
}

TEST_ADD(vfs_trunc, 0) {
  int n;
  void *wrbuf = malloc(4096);
  void *rdbuf = malloc(8196);
  fill_random(wrbuf, 4096);

  assert_open_ok(0, TESTPATH("file"), S_IWUSR | S_IRUSR, O_RDWR | O_CREAT);

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

  truncate(TESTPATH("file"), 0);
  assert_lseek_ok(0, 0, SEEK_SET);
  assert(read(3, rdbuf, 2048) == 0);

  free(wrbuf);
  free(rdbuf);
  unlink(TESTPATH("file"));

  syscall_fail(truncate(TESTDIR, 1023), EISDIR);
  return 0;
}

TEST_ADD(vfs_dir, 0) {
  syscall_fail(mkdir("/", 0), EEXIST);
  xmkdir(TESTPATH("test"), 0);
  syscall_fail(mkdir(TESTPATH("test"), 0), EEXIST);
  syscall_fail(mkdir(TESTPATH("/test///"), 0), EEXIST);
  xrmdir(TESTPATH("test"));
  xmkdir(TESTPATH("test"), 0);

  xmkdir(TESTPATH("/test2///"), 0);
  xaccess(TESTPATH("test2"), 0);

  xmkdir(TESTPATH("test3"), 0);
  xmkdir(TESTPATH("test3/subdir1"), 0);
  xmkdir(TESTPATH("test3/subdir2"), 0);
  xmkdir(TESTPATH("test3/subdir3"), 0);
  syscall_fail(mkdir(TESTPATH("test3/subdir1"), 0), EEXIST);
  xaccess(TESTPATH("test3/subdir2"), 0);

  syscall_fail(mkdir(TESTPATH("test4/subdir4"), 0), ENOENT);

  xrmdir(TESTPATH("test/"));
  xrmdir(TESTPATH("test2"));
  syscall_fail(rmdir(TESTPATH("test3")), ENOTEMPTY);
  xrmdir(TESTPATH("test3/subdir1"));
  xrmdir(TESTPATH("test3/subdir2"));
  xrmdir(TESTPATH("test3/subdir3"));
  xrmdir(TESTPATH("test3"));
  syscall_fail(rmdir(TESTPATH("test3")), ENOENT);
  syscall_fail(rmdir(TESTPATH("test4/subdir4")), ENOENT);

  syscall_fail(mkdir(TESTPATH("test3/subdir1"), 0), ENOENT);

  syscall_fail(mkdir("/", 0), EEXIST);
  syscall_fail(rmdir("/tmp"), EBUSY);

  return 0;
}

TEST_ADD(vfs_relative_dir, 0) {
  xchdir(TESTDIR);
  xmkdir("test", 0);
  syscall_fail(mkdir("test", 0), EEXIST);
  syscall_fail(mkdir("test///", 0), EEXIST);
  xchdir("test");

  xmkdir("test2///", 0);
  xrmdir("test2");

  xmkdir("test3", 0);
  xmkdir("test3/subdir1", 0);
  xmkdir("test3/subdir2", 0);
  xmkdir("test3/subdir3", 0);
  syscall_fail(mkdir("test3/subdir1", 0), EEXIST);
  xaccess("test3/subdir2", 0);

  xrmdir(TESTPATH("test/test3/subdir1"));
  xrmdir(TESTPATH("test/test3/subdir2"));
  xrmdir(TESTPATH("test/test3/subdir3"));
  xrmdir("test3");

  xchdir(TESTDIR);
  xrmdir("test");
  xchdir("/");
  return 0;
}

TEST_ADD(vfs_dot_dot_dir, 0) {
  xchdir(TESTDIR);

  xmkdir("test", 0);
  xchdir("test");
  xmkdir("test2///", 0);
  xchdir("test2");

  xchdir("..");
  xchdir("test2");

  xchdir("../test2");
  xchdir("../../");
  syscall_fail(mkdir("test", 0), EEXIST);

  xchdir("test");
  xrmdir("../test/test2");

  xchdir("./..");
  xrmdir("test");

  return 0;
}

TEST_ADD(vfs_dot_dir, 0) {
  syscall_fail(mkdir(TESTPATH("test/."), 0), ENOENT);
  syscall_fail(mkdir("/.", 0), EEXIST);
  syscall_fail(mkdir(TESTPATH("."), 0), EEXIST);

  return 0;
}

TEST_ADD(vfs_dot_dot_across_fs, 0) {
  xchdir("/../../../../");
  syscall_fail(mkdir("dev", 0), EEXIST);

  xchdir("dev/../dev/../../../dev/../../dev");
  syscall_fail(mkdir("../dev", 0), EEXIST);

  xchdir("../");
  syscall_fail(mkdir("dev", 0), EEXIST);

  return 0;
}

TEST_ADD(vfs_symlink_basic, 0) {
  xsymlink("Hello, world!", TESTPATH("testlink"));

  char buf[32];

  memset(buf, 0, sizeof(buf));
  assert(readlink(TESTPATH("testlink"), buf, 1024) == 13);
  string_eq("Hello, world!", buf);

  memset(buf, 0, sizeof(buf));
  assert(readlink(TESTPATH("testlink"), buf, 5) == 5);
  string_eq("Hello", buf);

  syscall_fail(symlink("Hello, world!", TESTPATH("testlink")), EEXIST);

  xunlink(TESTPATH("testlink"));

  return 0;
}

TEST_ADD(vfs_symlink_vnr, 0) {
  int n;
  struct stat sb;
  ino_t fileino;

  assert_open_ok(0, TESTPATH("file"), 0, O_RDWR | O_CREAT);
  xstat(TESTPATH("file"), &sb);
  fileino = sb.st_ino;

  /* Absolute symlink */
  xsymlink(TESTPATH("file"), TESTPATH("alink"));
  xstat(TESTPATH("alink"), &sb);
  assert(fileino == sb.st_ino);

  xsymlink(TESTPATH("alink"), TESTPATH("alink2"));
  xstat(TESTPATH("alink2"), &sb);
  assert(fileino == sb.st_ino);

  /* Relative symlink */
  xsymlink("file", TESTPATH("rlink"));
  xstat(TESTPATH("rlink"), &sb);
  assert(fileino == sb.st_ino);

  xsymlink("alink2", TESTPATH("rlink2"));
  xstat(TESTPATH("rlink2"), &sb);
  assert(fileino == sb.st_ino);

  /* Do not follow symlink */
  xlstat(TESTPATH("alink2"), &sb);
  assert(fileino != sb.st_ino);

  xunlink(TESTPATH("alink"));
  xunlink(TESTPATH("alink2"));
  xunlink(TESTPATH("rlink"));
  xunlink(TESTPATH("rlink2"));

  /* Symlink to directory */
  xsymlink("/tmp", TESTPATH("dlink"));
  xstat(TESTPATH("dlink/file"), &sb);
  assert(fileino == sb.st_ino);
  xunlink(TESTPATH("dlink"));

  /* Looped symlink */
  xsymlink(TESTPATH("slink"), TESTPATH("slink"));
  syscall_fail(stat(TESTPATH("slink"), &sb), ELOOP);
  xunlink(TESTPATH("slink"));

  /* Bad symlink */
  xsymlink(TESTPATH("nofile"), TESTPATH("blink"));
  syscall_fail(stat(TESTPATH("blink"), &sb), ENOENT);
  xunlink(TESTPATH("blink"));

  xunlink(TESTPATH("file"));

  return 0;
}

TEST_ADD(vfs_link, 0) {
  int n;
  struct stat sb;
  ino_t fileino;

  void *wrbuf = malloc(64);
  void *rdbuf = malloc(32);
  fill_random(wrbuf, 64);

  /* Create file and fill it with random data */
  assert_open_ok(0, TESTPATH("file"), S_IWUSR | S_IRUSR, O_RDWR | O_CREAT);
  xstat(TESTPATH("file"), &sb);
  assert(sb.st_nlink == 1);

  fileino = sb.st_ino;

  assert_write_ok(0, wrbuf, 32);

  /* Make a hard link */
  xlink(TESTPATH("file"), TESTPATH("file2"));
  xstat(TESTPATH("file2"), &sb);

  /* Ensure if inode number and link count is proper */
  assert(sb.st_ino == fileino);
  assert(sb.st_nlink == 2);

  /* Ensure if data is the same */
  assert_open_ok(1, TESTPATH("file2"), 0, O_RDWR);
  assert(read(4, rdbuf, 32) == 32);
  assert(!memcmp(wrbuf, rdbuf, 32));

  /* Make another link to the same file*/
  xlink(TESTPATH("file2"), TESTPATH("file3"));
  xstat(TESTPATH("file3"), &sb);

  /* Ensure if inode number and link count is proper */
  assert(sb.st_ino == fileino);
  assert(sb.st_nlink == 3);

  assert_open_ok(2, TESTPATH("file3"), 0, O_RDWR);

  /* Make a change to the first file and check for change*/
  assert_lseek_ok(0, 0, SEEK_SET);
  assert_write_ok(0, wrbuf + 32, 32);
  assert(read(5, rdbuf, 32) == 32);
  assert(!memcmp(wrbuf + 32, rdbuf, 32));

  /* Delete second file */
  assert_close_ok(1);
  xunlink(TESTPATH("file2"));

  xstat(TESTPATH("file"), &sb);
  assert(sb.st_nlink == 2);

  xunlink(TESTPATH("file"));

  xstat(TESTPATH("file3"), &sb);
  assert(sb.st_nlink == 1);

  xunlink(TESTPATH("file3"));

  syscall_fail(link("/tmp", "/tmp/foo"), EPERM);

  return 0;
}

TEST_ADD(vfs_chmod, 0) {
  struct stat sb;

  assert(xopen(TESTPATH("file"), O_RDWR | O_CREAT, 0) == 3);
  xstat(TESTPATH("file"), &sb);
  assert((sb.st_mode & ALLPERMS) == 0);

  xchmod(TESTPATH("file"), DEFFILEMODE);
  xstat(TESTPATH("file"), &sb);
  assert((sb.st_mode & ALLPERMS) == DEFFILEMODE);

  mode_t mode = S_IXGRP | S_IWOTH | S_IRUSR | S_ISUID;
  xchmod(TESTPATH("file"), mode);
  xstat(TESTPATH("file"), &sb);
  assert((sb.st_mode & ALLPERMS) == mode);

  mode_t lmode = S_IWUSR | S_IRWXU | S_IRWXO;
  xsymlink(TESTPATH("file"), TESTPATH("link"));
  xlchmod(TESTPATH("link"), lmode);
  xstat(TESTPATH("link"), &sb);
  assert((sb.st_mode & ALLPERMS) == mode);
  xlstat(TESTPATH("link"), &sb);
  assert((sb.st_mode & ALLPERMS) == lmode);

  xunlink(TESTPATH("file"));
  xunlink(TESTPATH("link"));

  return 0;
}
