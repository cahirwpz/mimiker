#include <stdc.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>
#include <vm_map.h>
#include <ktest.h>

static void dump_file(const char *path) {
  vnode_t *v;
  int res = vfs_lookup(path, &v);
  assert(res == 0);

  char buffer[1000];
  memset(buffer, '\0', sizeof(buffer));
  uio_t uio;
  iovec_t iov;
  uio.uio_op = UIO_READ;

  /* Read entire file - even too much. */
  uio.uio_iovcnt = 1;
  uio.uio_vmspace = get_kernel_vm_map();
  uio.uio_iov = &iov;
  uio.uio_offset = 0;
  iov.iov_base = buffer;
  iov.iov_len = sizeof(buffer);
  uio.uio_resid = sizeof(buffer);

  res = VOP_READ(v, &uio);

  kprintf("file %s:\n%s\n", path, buffer);
}

static int test_ramdisk() {
  ramdisk_dump();
  dump_file("/initrd/tests/initrd/directory1/file1");
  dump_file("/initrd/tests/initrd/directory1/file2");
  dump_file("/initrd/tests/initrd/directory1/file3");
  dump_file("/initrd/tests/initrd/directory2/file1");
  dump_file("/initrd/tests/initrd/directory2/file2");
  dump_file("/initrd/tests/initrd/directory2/file3");
  dump_file("/initrd/tests/initrd/random_file.txt");
  dump_file("/initrd/tests/initrd/some_file.exe");
  dump_file("/initrd/tests/initrd/just_file.cpp");
  dump_file("/initrd/tests/initrd/some_file.c");
  dump_file("/initrd/tests/initrd/just_file.hs");
  dump_file(
    "/initrd/tests/initrd/very/very/very/very/very/very/very/very/very/deep/"
    "directory/deep_inside");
  dump_file("/initrd/tests/initrd/short_file_name/a");
  dump_file("/initrd/tests/initrd/level1/level2/level3/level123");
  dump_file("/initrd/tests/initrd/level1/level2/level12");
  dump_file("/initrd/tests/initrd/level1/level1");
  return KTEST_SUCCESS;
}

KTEST_ADD(ramdisk, test_ramdisk, 0);
