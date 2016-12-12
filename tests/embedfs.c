#include <embedfs.h>
#include <mount.h>
#include <stdc.h>
#include <vnode.h>
#include <uio.h>
#include <vm_map.h>

EMBED_FILE("embed_test.txt", embedfs_txt);

int main() {
  vnode_t *v;
  int res = vfs_lookup("/embed/embed_test.txt", &v);
  assert(res == 0);

  char buffer[1000];
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
  assert(res == 193);
  assert(strncmp(buffer + 20, "le, which is filled ", 20) == 0);

  return 0;
}
