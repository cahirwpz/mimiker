#include <sys/mimiker.h>
#include <sys/dirent.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/vnode.h>

int readdir_generic(vnode_t *v, uio_t *uio, readdir_ops_t *ops) {
  dirent_t *dir;
  void *it = DIRENT_DOT;
  off_t offset = 0;
  int error;

  /* Locate proper directory based on offset */
  for (; it; it = ops->next(v, it)) {
    unsigned reclen = _DIRENT_RECLEN(dir, ops->namlen_of(v, it));
    if (offset + reclen > (unsigned)uio->uio_offset) {
      assert(it == NULL || offset == uio->uio_offset);
      break;
    }
    offset += reclen;
  }

  for (; it; it = ops->next(v, it)) {
    unsigned namlen = ops->namlen_of(v, it);
    unsigned reclen = _DIRENT_RECLEN(dir, namlen);

    if (uio->uio_resid < reclen)
      break;

    dir = kmalloc(M_TEMP, reclen, M_ZERO);
    dir->d_namlen = namlen;
    dir->d_reclen = reclen;
    ops->convert(v, it, dir);
    error = uiomove(dir, reclen, uio);
    kfree(M_TEMP, dir);
    if (error)
      return error;
  }

  return 0;
}
