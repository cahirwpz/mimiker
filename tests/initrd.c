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

  char buffer[500];
  memset(buffer, '\0', sizeof(buffer));

  uio_t uio;
  uio = UIO_SINGLE_KERNEL(UIO_READ, 0, buffer, sizeof(buffer) - 1);
  res = VOP_READ(v, &uio);

  kprintf("file %s:\n%s\n", path, buffer);
}

static int test_ramdisk() {
  dump_file("/usr/include/sys/errno.h");
  dump_file("/usr/include/sys/dirent.h");
  dump_file("/usr/include/sys/unistd.h");
  return KTEST_SUCCESS;
}

KTEST_ADD(ramdisk, test_ramdisk, 0);

/* Completing this test takes far too much time due to its verbosity - so it's
   disabled with the BROKEN flag. */
static int test_ramdisk_dump() {
  ramdisk_dump();
  return KTEST_SUCCESS;
}
KTEST_ADD(ramdisk_dump, test_ramdisk_dump, KTEST_FLAG_BROKEN);
