#include <stdc.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>
#include <vm_map.h>
#include <ktest.h>
#include <dirent.h>

static void dump_file(const char *path) {
  vnode_t *v;
  int res = vfs_lookup(path, &v);
  assert(res == 0);

  char buffer[500];
  memset(buffer, '\0', sizeof(buffer));

  uio_t uio;
  uio = UIO_SINGLE_KERNEL(UIO_READ, 0, buffer, sizeof(buffer));
  res = VOP_READ(v, &uio);

  kprintf("file %s:\n%s\n", path, buffer);
}

void dump_dirent(dirent_t *dir) {
  kprintf("%s\n", dir->d_name);
}

void dump_directory(const char *path) {
  vnode_t *v;
  int res = vfs_lookup(path, &v);
  assert(res == 0);

  char buffer[50];
  memset(buffer, '\0', sizeof(buffer));

  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 0, buffer, sizeof(buffer));
  int bytes = 0;

  kprintf("Contents of directory: %s\n", path);

  uio.uio_offset = 0;

  do {
    bytes = VOP_READDIR(v, &uio);

    /* Dump directories inside buffer */
    dirent_t *dir = (dirent_t *)((void *)buffer);
    for (int off = 0; off < bytes; off += dir->d_reclen) {
      if (dir->d_reclen) {
        dump_dirent(dir);
        dir = _DIRENT_NEXT(dir);
      } else
        break;
    }
    int old_offset = uio.uio_offset;
    uio = UIO_SINGLE_KERNEL(UIO_READ, old_offset, buffer, sizeof(buffer));
  } while (bytes > 0);
}

static int test_readdir() {
  dump_directory("/usr/include/");
  return KTEST_SUCCESS;
}

static int test_ramdisk() {
  dump_file("/usr/include/sys/errno.h");
  dump_file("/usr/include/sys/dirent.h");
  dump_file("/usr/include/sys/unistd.h");
  return KTEST_SUCCESS;
}

KTEST_ADD(ramdisk, test_ramdisk, 0);
KTEST_ADD(readdir, test_readdir, 0);

/* Completing this test takes far too much time due to its verbosity - so it's
   disabled with the BROKEN flag. */
static int test_ramdisk_dump() {
  ramdisk_dump();
  return KTEST_SUCCESS;
}
KTEST_ADD(ramdisk_dump, test_ramdisk_dump, KTEST_FLAG_BROKEN);
