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
  ktest_assert(error == -ENOENT);
  error = vfs_lookup("/usr", &v);
  ktest_assert(error == -ENOENT); /* Root filesystem not implemented yet. */
  error = vfs_lookup("/", &v);
  ktest_assert(error == 0 && v == vfs_root_vnode);
  vnode_unref(v);
  error = vfs_lookup("/dev////", &v);
  ktest_assert(error == 0 && v == vfs_root_dev_vnode);
  vnode_unref(v);

  vnode_t *dev_null, *dev_zero;
  error = vfs_lookup("/dev/null", &dev_null);
  ktest_assert(error == 0);
  vnode_unref(dev_null);
  error = vfs_lookup("/dev/zero", &dev_zero);
  ktest_assert(error == 0);
  vnode_unref(dev_zero);

  ktest_assert(dev_zero->v_usecnt == 1);
  /* Ask for the same vnode multiple times and check for correct v_usecnt. */
  error = vfs_lookup("/dev/zero", &dev_zero);
  ktest_assert(error == 0);
  error = vfs_lookup("/dev/zero", &dev_zero);
  ktest_assert(error == 0);
  error = vfs_lookup("/dev/zero", &dev_zero);
  ktest_assert(error == 0);
  ktest_assert(dev_zero->v_usecnt == 4);
  vnode_unref(dev_zero);
  vnode_unref(dev_zero);
  vnode_unref(dev_zero);
  ktest_assert(dev_zero->v_usecnt == 1);

  uio_t uio;
  iovec_t iov;
  int res = 0;
  char buffer[100];
  memset(buffer, '=', sizeof(buffer));

  /* Perform a READ test on /dev/zero, cleaning buffer. */
  uio.uio_op = UIO_READ;
  uio.uio_vmspace = get_kernel_vm_map();
  iov.iov_base = buffer;
  iov.iov_len = sizeof(buffer);
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_offset = 0;
  uio.uio_resid = sizeof(buffer);

  res = VOP_READ(dev_zero, &uio);
  ktest_assert(res == 0);
  ktest_assert(buffer[1] == 0 && buffer[10] == 0);
  ktest_assert(uio.uio_resid == 0);

  /* Now write some data to /dev/null */
  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_kernel_vm_map();
  iov.iov_base = buffer;
  iov.iov_len = sizeof(buffer);
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_offset = 0;
  uio.uio_resid = sizeof(buffer);

  ktest_assert(dev_null != 0);
  res = VOP_WRITE(dev_null, &uio);
  ktest_assert(res == 0);
  ktest_assert(uio.uio_resid == 0);

  /* Test writing to UART */
  vnode_t *dev_uart;
  error = vfs_lookup("/dev/uart", &dev_uart);
  ktest_assert(error == 0);
  char *str = "Some string for testing UART write\n";

  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_kernel_vm_map();
  iov.iov_base = str;
  iov.iov_len = strlen(str);
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_offset = 0;
  uio.uio_resid = iov.iov_len;

  res = VOP_WRITE(dev_uart, &uio);
  ktest_assert(res == 0);

  return KTEST_SUCCESS;
}

KTEST_ADD(vfs, test_vfs, 0);
