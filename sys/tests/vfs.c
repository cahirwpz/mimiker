#include <sys/mount.h>
#include <sys/libkern.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/vm_map.h>
#include <sys/ktest.h>

static bool fsname_of(vnode_t *v, const char *fsname) {
  return strncmp(v->v_mount->mnt_vfc->vfc_name, fsname, strlen(fsname)) == 0;
}

static int test_vfs(void) {
  vnode_t *v;
  int error;

  error = vfs_lookup("/dev/SPAM", &v);
  assert(error == ENOENT);
  error = vfs_lookup("/", &v);
  assert(error == 0);
  assert(fsname_of(v, "initrd"));
  vnode_drop(v);
  error = vfs_lookup("/dev////", &v);
  assert(error == 0);
  assert(fsname_of(v, "devfs"));
  vnode_drop(v);

  error = vfs_lookup("/dev", &v);
  assert(error == 0 && !is_mountpoint(v));
  vnode_drop(v);

  vnode_t *dev_null, *dev_zero;
  error = vfs_lookup("/dev/null", &dev_null);
  assert(error == 0);
  vnode_drop(dev_null);
  error = vfs_lookup("/dev/zero", &dev_zero);
  assert(error == 0);
  vnode_drop(dev_zero);

  assert(dev_zero->v_usecnt == 1);
  /* Ask for the same vnode multiple times and check for correct v_usecnt. */
  error = vfs_lookup("/dev/zero", &dev_zero);
  assert(error == 0);
  error = vfs_lookup("/dev/zero", &dev_zero);
  assert(error == 0);
  error = vfs_lookup("/dev/zero", &dev_zero);
  assert(error == 0);
  assert(dev_zero->v_usecnt == 4);
  vnode_drop(dev_zero);
  vnode_drop(dev_zero);
  vnode_drop(dev_zero);
  assert(dev_zero->v_usecnt == 1);

  uio_t uio;
  int res = 0;
  char buffer[100];
  memset(buffer, '=', sizeof(buffer));

  /* Perform a READ test on /dev/zero, cleaning buffer. */
  uio = UIO_SINGLE_KERNEL(UIO_READ, 0, buffer, sizeof(buffer));
  res = VOP_READ(dev_zero, &uio);
  assert(res == 0);
  assert(buffer[1] == 0 && buffer[10] == 0);
  assert(uio.uio_resid == 0);

  /* Now write some data to /dev/null */
  assert(dev_null != 0);
  uio = UIO_SINGLE_KERNEL(UIO_WRITE, 0, buffer, sizeof(buffer));
  res = VOP_WRITE(dev_null, &uio);
  assert(res == 0);
  assert(uio.uio_resid == 0);

  /* Test writing to UART */
  vnode_t *dev_cons;
  error = vfs_lookup("/dev/cons", &dev_cons);
  assert(error == 0);
  char *str = "Some string for testing UART write\n";

  uio = UIO_SINGLE_KERNEL(UIO_WRITE, 0, str, strlen(str));
  res = VOP_WRITE(dev_cons, &uio);
  assert(res == 0);

  return KTEST_SUCCESS;
}

KTEST_ADD(vfs, test_vfs, 0);
