#include <mount.h>
#include <stdc.h>
#include <vnode.h>
#include <errno.h>
#include <vm_map.h>

int main() {
  vnode_t *v;
  int error;

  error = vfs_lookup("/dev/SPAM", &v);
  assert(error == ENOENT);
  error = vfs_lookup("/usr", &v);
  assert(error == ENOTSUP); /* Root filesystem not implemented yet. */
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
  assert(res == 0);
  assert(buffer[1] == 0 && buffer[10] == 0);
  assert(uio.uio_resid == 0);

  /* Now write some data to /dev/null */
  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_kernel_vm_map();
  iov.iov_base = buffer;
  iov.iov_len = sizeof(buffer);
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_offset = 0;
  uio.uio_resid = sizeof(buffer);

  assert(dev_null != 0);
  res = VOP_WRITE(dev_null, &uio);
  assert(res == 0);
  assert(uio.uio_resid == 0);

  /* Test writing to UART */
  vnode_t *dev_uart;
  error = vfs_lookup("/dev/uart", &dev_uart);
  assert(error == 0);
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
  assert(res > 0);

  return 0;
}
