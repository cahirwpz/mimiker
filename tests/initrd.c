#include <stdc.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>
#include <vm_map.h>
#include <ktest.h>
#include <dirent.h>
#include <malloc.h>

static MALLOC_DEFINE(M_INITRD_TEST, "initrd test", 16, 128);

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

/* Perform BFS on directory tree and print all files,
 * normally we'd want to perform DFS, but we are in kernel space
 * where due to very small stack, we want to avoid recursion */
static void dump_directory(const char *start_path) {
  kprintf("Contents of directory tree rooted at %s:\n", start_path);
  struct dirent_qnode {
    TAILQ_ENTRY(dirent_qnode) link;
    char *path;
  };

  TAILQ_HEAD(, dirent_qnode) queue;
  TAILQ_INIT(&queue);

  struct dirent_qnode *start_node =
    kmalloc(M_INITRD_TEST, sizeof(struct dirent_qnode), 0);
  start_node->path =
    kstrndup(M_INITRD_TEST, start_path, strlen(start_path) + 1);
  TAILQ_INSERT_TAIL(&queue, start_node, link);

  while (!TAILQ_EMPTY(&queue)) {
    struct dirent_qnode *cur = TAILQ_FIRST(&queue);

    vnode_t *v;
    int res = vfs_lookup(cur->path, &v);
    assert(res == 0);

    char buffer[50];
    memset(buffer, '\0', sizeof(buffer));
    uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 0, buffer, sizeof(buffer));
    int bytes = 0;
    uio.uio_offset = 0;

    do {
      bytes = VOP_READDIR(v, &uio);
      dirent_t *dir = (dirent_t *)((void *)buffer);

      for (int off = 0; off < bytes; off += dir->d_reclen) {
        if (dir->d_reclen) {
          kprintf("%s%s\n", cur->path, dir->d_name);
          if (dir->d_type & DT_DIR && strcmp(dir->d_name, ".") != 0 &&
              strcmp(dir->d_name, "..") != 0) {
            /* Prepare new name */
            int new_path_len = strlen(cur->path) + dir->d_namlen + 10;
            char *new_path = kmalloc(M_INITRD_TEST, new_path_len, 0);
            new_path[0] = '\0';
            strlcat(new_path, cur->path, new_path_len);
            strlcat(new_path, dir->d_name, new_path_len);
            strlcat(new_path, "/", new_path_len);

            /* Prepare new node */
            struct dirent_qnode *new_node =
              kmalloc(M_INITRD_TEST, sizeof(struct dirent_qnode), 0);
            new_node->path = new_path;
            TAILQ_INSERT_TAIL(&queue, new_node, link);
          }
          dir = _DIRENT_NEXT(dir);
        } else
          break;
      }
      int old_offset = uio.uio_offset;
      uio = UIO_SINGLE_KERNEL(UIO_READ, old_offset, buffer, sizeof(buffer));
    } while (bytes > 0);

    TAILQ_REMOVE(&queue, cur, link);
    kfree(M_INITRD_TEST, cur->path);
    kfree(M_INITRD_TEST, cur);
  }
}

static int test_readdir() {
  dump_directory("/usr/");
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
