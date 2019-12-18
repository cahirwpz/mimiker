#include <sys/klog.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/vm_map.h>
#include <sys/malloc.h>
#include <sys/libkern.h>
#include <sys/vfs.h>

int vfs_name_in_dir(vnode_t *dv, vnode_t *v, char *buf, size_t *lastp) {
  int error = 0;

  vattr_t va;
  if ((error = VOP_GETATTR(v, &va)))
    return error;

  int offset = 0;
  size_t last = *lastp;
  uio_t uio;

  dirent_t *dirents = kmalloc(M_TEMP, PATH_MAX, 0);

  for (;;) {
    uio = UIO_SINGLE_KERNEL(UIO_READ, offset, dirents, PATH_MAX);
    if ((error = VOP_READDIR(dv, &uio, NULL)))
      goto end;

    intptr_t nread = uio.uio_offset - offset;
    if (nread == 0)
      break;

    /* Look for dirent with matching inode number. */
    for (dirent_t *de = dirents; (void *)de < (void *)dirents + nread;
         de = _DIRENT_NEXT(de)) {
      if (de->d_fileno != va.va_ino)
        continue;
      if (last < de->d_namlen) {
        error = ENAMETOOLONG;
      } else {
        last -= de->d_namlen;
        memcpy(&buf[last], de->d_name, de->d_namlen);
      }
      goto end;
    }
  }
  error = ENOENT;

end:
  *lastp = last;
  kfree(M_TEMP, dirents);
  return error;
}
