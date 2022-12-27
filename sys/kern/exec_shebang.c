#define KL_LOG KL_PROC
#include <sys/klog.h>
#define _EXEC_IMPL
#include <sys/exec.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/syslimits.h>
#include <sys/malloc.h>
#include <sys/vm_map.h>
#include <sys/vnode.h>

#define SHEBANG "#!"

int exec_shebang_inspect(vnode_t *vn) {
  int error;
  char sig[sizeof(SHEBANG)];
  vattr_t attr;

  if ((error = VOP_GETATTR(vn, &attr)))
    return error;

  if (attr.va_size < 2)
    return ENOEXEC;

  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 0, sig, 2);
  if ((error = VOP_READ(vn, &uio))) {
    klog("Failed to read shebang!");
    return error;
  }
  assert(uio.uio_resid == 0);

  /* Check for the magic header. */
  if ((sig[0] != SHEBANG[0]) || (sig[1] != SHEBANG[1]))
    return ENOEXEC;

  return 0;
}

static int set_interp(exec_args_t *args, char *str) {
  size_t copied = strlcpy(args->end, str, args->left);
  if (copied >= args->left)
    return E2BIG;
  args->interp = args->end;
  args->end += copied;
  args->left -= copied;

  /* HACK Let's use one entry before real argv (allocated in exec_args_init)
   * to store interpreter path. */
  args->argc++;
  args->argv--;
  args->argv[0] = args->interp;
  args->argv[1] = args->path; /* pass full path to the interpreter */
  return 0;
}

int exec_shebang_load(vnode_t *vn, exec_args_t *args) {
  char *interp = kmalloc(M_TEMP, PATH_MAX, 0);
  int error;

  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 2, interp, PATH_MAX);
  if ((error = VOP_READ(vn, &uio)))
    goto fail;

  /* Make sure there's a terminator in user data. */
  interp[PATH_MAX - 1] = '\0';
  /* Interpreter spans characters up to first space. */
  size_t span = strcspn(interp, " \t\n");
  interp[span] = '\0';

  error = set_interp(args, interp);

fail:
  kfree(M_TEMP, interp);
  return error;
}
