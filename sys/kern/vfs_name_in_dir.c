#include <sys/klog.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/vm_map.h>
#include <sys/malloc.h>
#include <sys/libkern.h>
#include <sys/vfs.h>

int vfs_name_in_dir(vnode_t *dv, vnode_t *v, char *buf, size_t *lenp) {
  int error = 0;

  vattr_t va;
  if ((error = VOP_GETATTR(v, &va)))
    return error;

  int offset = 0;
  int nread = 0;
  uio_t uio;

  char *dirents = kmalloc(M_TEMP, PATH_MAX, 0);

  do {
    uio = UIO_SINGLE_KERNEL(UIO_READ, offset, dirents, PATH_MAX);
    if ((error = VOP_READDIR(dv, &uio, NULL)))
      goto end;
    nread = uio.uio_offset - offset;

    for (dirent_t *de = (dirent_t *)dirents; (char *)de < dirents + nread;
         de = (dirent_t *)((char *)de + de->d_reclen)) {
      if (de->d_fileno == va.va_ino) {
        size_t len = strlen(de->d_name);
        if (*lenp < len) {
          error = ENAMETOOLONG;
        } else {
          *lenp -= len;
          memcpy(buf + *lenp, de->d_name, len);
        }
        goto end;
      }

      if (de->d_reclen == 0)
        panic("Failed to find child node in parent directory");
    }
  } while (nread > 0);
  panic("Failed to find child node in parent directory");
end:
  kfree(M_TEMP, dirents);
  return error;
}
