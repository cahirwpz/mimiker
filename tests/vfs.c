#include <mount.h>
#include <stdc.h>
#include <vnode.h>
#include <errno.h>
#include <vm_map.h>
#include <ktest.h>

static int test_vfs() {
  vnode_t *v;
  int error;

  error = vfs_lookup("/dev/SPAM", &v);
  assert(error == -ENOENT);
  error = vfs_lookup("/", &v);
  assert(error == 0 && v == vfs_root_vnode);
  vnode_unref(v);
  error = vfs_lookup("/dev////", &v);
  assert(error == 0 && v == vfs_root_dev_vnode);
  vnode_unref(v);

  vnode_t *dev_null, *dev_zero;
  error = vfs_lookup("/dev/null", &dev_null);
  assert(error == 0);
  vnode_unref(dev_null);
  error = vfs_lookup("/dev/zero", &dev_zero);
  assert(error == 0);
  vnode_unref(dev_zero);

  assert(dev_zero->v_usecnt == 1);
  /* Ask for the same vnode multiple times and check for correct v_usecnt. */
  error = vfs_lookup("/dev/zero", &dev_zero);
  assert(error == 0);
  error = vfs_lookup("/dev/zero", &dev_zero);
  assert(error == 0);
  error = vfs_lookup("/dev/zero", &dev_zero);
  assert(error == 0);
  assert(dev_zero->v_usecnt == 4);
  vnode_unref(dev_zero);
  vnode_unref(dev_zero);
  vnode_unref(dev_zero);
  assert(dev_zero->v_usecnt == 1);

  uio_t uio;
  int res = 0;

  char buffer[100];
  memset(buffer, '=', sizeof(buffer));

  /* Perform a READ test on /dev/zero, cleaning buffer. */
  prepare_single_kernel_uio(&uio, UIO_READ, 0, buffer, sizeof(buffer));

  res = VOP_READ(dev_zero, &uio);
  assert(res == 0);
  assert(buffer[1] == 0 && buffer[10] == 0);
  assert(uio.uio_resid == 0);

  /* Now write some data to /dev/null */
  prepare_single_kernel_uio(&uio, UIO_WRITE, 0, buffer, sizeof(buffer));

  assert(dev_null != 0);
  res = VOP_WRITE(dev_null, &uio);
  assert(res == 0);
  assert(uio.uio_resid == 0);

  /* Test writing to UART */
  vnode_t *dev_cons;
  error = vfs_lookup("/dev/cons", &dev_cons);
  assert(error == 0);
  char *str = "Some string for testing UART write\n";

  prepare_single_kernel_uio(&uio, UIO_WRITE, 0, str, strlen(str));

  res = VOP_WRITE(dev_cons, &uio);
  assert(res == 0);

  return KTEST_SUCCESS;
}

KTEST_ADD(vfs, test_vfs, 0);
