#include <stdc.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>
#include <vm_map.h>
#include <ktest.h>

static void dump_file(const char *path) {
  vnode_t *v;
  int res = vfs_lookup(path, &v);
  // int res = vfs_lookup("/initrd/empty_file", &v);
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

  dump_file("/initrd/some_dir/nested_dir/message");
  dump_file("/initrd/some_dir/nested_dir/file5");
  dump_file("/initrd/empty_file");
  dump_file("/initrd/file1");
  dump_file("/initrd/file2");
  dump_file("/initrd/file3");
  dump_file("/initrd/file4");
  dump_file("/initrd/just_another_dir/file6");
  return 0;
}

KTEST_ADD(ramdisk, test_ramdisk, KTEST_FLAG_BROKEN);
