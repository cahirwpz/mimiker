#define KL_LOG KL_PROC
#include <klog.h>
#include <exec.h>
#include <stdc.h>
#include <syslimits.h>
#include <malloc.h>
#include <vm_map.h>
#include <vnode.h>

#define SHEBANG "#!"

int exec_shebang_inspect(vnode_t *vn) {
  int error;
  char sig[sizeof(SHEBANG)];
  vattr_t attr;

  if ((error = VOP_GETATTR(vn, &attr)))
    return error;

  if (attr.va_size < 2)
    return 0;

  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 0, sig, 2);
  if ((error = VOP_READ(vn, &uio)) < 0) {
    klog("Failed to read shebang!");
    return error;
  }
  assert(uio.uio_resid == 0);

  /* Check for the magic header. */
  return (sig[0] == SHEBANG[0]) && (sig[1] == SHEBANG[1]);
}

int exec_shebang_interp(vnode_t *vn, char **interp_p) {
  char *interp = kmalloc(M_TEMP, PATH_MAX, 0);
  int error;

  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 2, interp, PATH_MAX);
  if ((error = VOP_READ(vn, &uio)) < 0) {
    kfree(M_TEMP, interp);
    return error;
  }

  /* Make sure there's a terminator in user data. */
  interp[PATH_MAX - 1] = '\0';
  /* Interpreter spans characters up to first space. */
  size_t span = strcspn(interp, " \t\n");
  interp[span] = '\0';

  *interp_p = interp;
  return 0;
}
