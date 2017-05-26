#include <common.h>
#include <dirent.h>
#include <malloc.h>
#include <uio.h>
#include <vnode.h>

int readdir_generic(vnode_t *v, uio_t *uio, readdir_ops_t *ops) {
  void *it = ops->first(v);
  dirent_t *dir;
  off_t offset = 0;
  int error;

  /* Locate proper directory based on offset */
  for (; it; it = ops->next(it)) {
    unsigned reclen = _DIRENT_RECLEN(dir, ops->namlen_of(it));
    if (offset + reclen > (unsigned)uio->uio_offset) {
      assert(it == NULL || offset == uio->uio_offset);
      break;
    }
    offset += reclen;
  }

  for (; it; it = ops->next(it)) {
    unsigned namlen = ops->namlen_of(it);
    unsigned reclen = _DIRENT_RECLEN(dir, namlen);

    if (uio->uio_resid < reclen)
      break;

    dir = kmalloc(M_TEMP, reclen, M_ZERO);
    dir->d_namlen = namlen;
    dir->d_reclen = reclen;
    ops->convert(it, dir);
    error = uiomove(dir, reclen, uio);
    kfree(M_TEMP, dir);
    if (error < 0)
      return -error;
  }

  return uio->uio_offset - offset;
}
