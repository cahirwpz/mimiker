#include <sys/errno.h>
#include <sys/ktest.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/devfs.h>

static int test_devfs(void) {
  vnode_t *v, *v2;
  int error;
  devfs_node_t *d;

  /* Directory creation and removal */
  error = devfs_makedir(NULL, "testdir", &d);
  assert(error == 0);
  error = vfs_namelookup("/dev/testdir", &v);
  assert(error == 0);
  /* One reference from devfs, one from us. */
  assert(v->v_usecnt == 2);
  assert(v->v_type == V_DIR);
  error = devfs_unlink(d);
  assert(error == 0);
  error = vfs_namelookup("/dev/testdir", &v2);
  assert(error == ENOENT);
  /* We're still holding a reference, so the vnode and devfs node should still
   * be around. */
  assert(v->v_usecnt == 1);
  (void)(*(volatile char *)d); /* Access d to trigger KASAN if it was freed. */
  vnode_drop(v);

  /* Now the vnode and devfs node should be freed... but we can't test it. */

  return KTEST_SUCCESS;
}

KTEST_ADD(devfs, test_devfs, 0);
