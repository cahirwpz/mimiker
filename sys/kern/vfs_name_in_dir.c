#include <sys/klog.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/vm_map.h>
#include <sys/malloc.h>
#include <sys/libkern.h>
#include <sys/vfs.h>

int vfs_name_in_dir(vnode_t *dv, vnode_t *v, char *bufp, size_t *lenp) {
  int error = 0;

  vattr_t va;
  if ((error = VOP_GETATTR(v, &va)))
    return error;

  int offset = 0;
  int nread = 0;
  uio_t uio;

  char *buf = kmalloc(M_TEMP, PATH_MAX, 0);

  do {
    uio = UIO_SINGLE_KERNEL(UIO_READ, offset, buf, PATH_MAX);
    if ((error = VOP_READDIR(dv, &uio, NULL)))
      goto end;
    nread = uio.uio_offset - offset;

    for (dirent_t *dir = (dirent_t *)buf; (char *)dir < buf + nread;
         dir = (dirent_t *)((char *)dir + dir->d_reclen)) {
      if (dir->d_fileno == va.va_ino) {
        size_t len = strlen(dir->d_name);
        if (*lenp < len) {
          error = ENAMETOOLONG;
        } else {
          *lenp -= len;
          memcpy(bufp + *lenp, dir->d_name, len);
        }
        goto end;
      }

      if (dir->d_reclen == 0)
        panic("Failed to find child node in parent directory");
    }
  } while (nread > 0);
  panic("Failed to find child node in parent directory");
end:
  kfree(M_TEMP, buf);
  return error;
}
