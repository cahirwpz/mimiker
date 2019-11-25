
#include <sys/klog.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/vm_map.h>
#include <sys/malloc.h>
#include <sys/libkern.h>

int getcwd_scandir(vnode_t *lvp, vnode_t *uvp, char **bpp, char *bufp) {
  int error = 0;
  char *buf = kmalloc(M_TEMP, 1024, 0);

  vattr_t va;
  VOP_GETATTR(uvp, &va);

  int offset = 0;
  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, offset, buf, 1024);
  VOP_READDIR(lvp, &uio, NULL);

  while (uio.uio_offset != offset) {
    int nread = uio.uio_offset - offset;
    for (dirent_t *dir = (dirent_t *)buf; (char *)dir < buf + nread;
         dir = (dirent_t *)((char *)dir + dir->d_reclen)) {
      if (dir->d_fileno == va.va_ino) {
        size_t len = strlen(dir->d_name);
        if (*bpp - len - 1 < bufp) {
          error = ENAMETOOLONG;
        } else {
          *bpp -= len;
          memcpy(*bpp, dir->d_name, len);
          *bpp -= 1;
          **bpp = '/';
        }
        goto end;
      }

      if (dir->d_reclen == 0) {
        panic("Failed to find child node in parent directory");
      }
    }

    offset = uio.uio_offset;
    uio = UIO_SINGLE_KERNEL(UIO_READ, offset, buf, 1024);
    VOP_READDIR(lvp, &uio, NULL);
  }
end:
  kfree(M_TEMP, buf);
  return error;
}
