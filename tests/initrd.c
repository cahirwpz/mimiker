#include <stdc.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>
#include <vm_map.h>
#include <ktest.h>
#include <dirent.h>

static void prepare_uio(uio_t *uio, iovec_t *iov, char *buffer, int buf_size)
{
  uio->uio_op = UIO_READ;

  /* Read entire file - even too much. */
  uio->uio_iovcnt = 1;
  uio->uio_vmspace = get_kernel_vm_map();
  uio->uio_iov = iov;
  uio->uio_offset = 0;
  iov->iov_base = buffer;
  iov->iov_len = buf_size;
  uio->uio_resid = buf_size;
}

static void dump_file(const char *path) {
  vnode_t *v;
  int res = vfs_lookup(path, &v);
  assert(res == 0);

  char buffer[1000];
  memset(buffer, '\0', sizeof(buffer));
  uio_t uio;
  iovec_t iov;

  prepare_uio(&uio, &iov, buffer, sizeof(buffer));

  res = VOP_READ(v, &uio);

  kprintf("file %s:\n%s\n", path, buffer);
}

void dump_dirent(dirent_t *dir)
{
  kprintf("%s\n", dir->d_name);
}

//void dump_directory(const char *path)
//{
//  vnode_t *v;
//  int res = vfs_lookup(path, &v);
//  assert(res == 0);
//
//  char buffer[50];
//  memset(buffer, '\0', sizeof(buffer));
//  uio_t uio;
//  iovec_t iov;
//
//  prepare_uio(&uio, &iov, buffer, sizeof(buffer));
//  int bytes = 0;
//
//  kprintf("Contents of directory: %s\n", path);
//
//  do
//  {
//    bytes = VOP_READDIR(v, &uio);
//    uio.uio_offset = 0;
//
//    /* Dump directories inside buffer */
//    dirent_t *dir = buffer;
//    for(int off = 0; off < bytes; off += dir->d_reclen)
//    {
//      dump_dirent(dir);
//      dir = _DIRENT_NEXT(dir);
//    }
//    uio.uio_offset = 0;
//  } while(bytes > 0);
//
//  return KTEST_SUCCESS;
//}
//
///* TODO this test should be moved to */
//static int test_readdir() {
//  dump_directory("/initrd/directory1/");
//  dump_directory("/initrd/");
//  return 0;
//}

static int test_ramdisk() {
  ramdisk_dump();
  dump_file("/tests/initrd/directory1/file1");
  dump_file("/tests/initrd/directory1/file2");
  dump_file("/tests/initrd/directory1/file3");
  dump_file("/tests/initrd/directory2/file1");
  dump_file("/tests/initrd/directory2/file2");
  dump_file("/tests/initrd/directory2/file3");
  dump_file("/tests/initrd/random_file.txt");
  dump_file("/tests/initrd/some_file.exe");
  dump_file("/tests/initrd/just_file.cpp");
  dump_file("/tests/initrd/some_file.c");
  dump_file("/tests/initrd/just_file.hs");
  dump_file("/tests/initrd/very/very/very/very/very/very/very/very/very/deep/"
            "directory/deep_inside");
  dump_file("/tests/initrd/short_file_name/a");
  dump_file("/tests/initrd/level1/level2/level3/level123");
  dump_file("/tests/initrd/level1/level2/level12");
  dump_file("/tests/initrd/level1/level1");
  return KTEST_SUCCESS;
}

KTEST_ADD(ramdisk, test_ramdisk, 0);
