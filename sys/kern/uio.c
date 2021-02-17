#include <sys/klog.h>
#include <sys/uio.h>
#include <sys/libkern.h>
#include <sys/vm_map.h>
#include <sys/malloc.h>
#include <sys/errno.h>

static int copyin_vmspace(vm_map_t *vm, const void *restrict udaddr,
                          void *restrict kaddr, size_t len) {
  if (vm == vm_map_kernel()) {
    memcpy(kaddr, udaddr, len);
    return 0;
  }

  if (vm == vm_map_user())
    return copyin(udaddr, kaddr, len);

  panic("copyin on non-active vm maps is not supported");
}

static int copyout_vmspace(vm_map_t *vm, const void *restrict kaddr,
                           void *restrict udaddr, size_t len) {
  if (vm == vm_map_kernel()) {
    memcpy(udaddr, kaddr, len);
    return 0;
  }

  if (vm == vm_map_user())
    return copyout(kaddr, udaddr, len);

  panic("copyout on non-active vm maps is not supported");
}

/* Heavily inspired by NetBSD's uiomove */
/* This function modifies uio to reflect on the progress. */
int uiomove(void *buf, size_t n, uio_t *uio) {
  /* Calling uiomove from critical section (no interrupts or no preemption)
   * is not allowed since it may be copying from pageable memory. */
  assert(!intr_disabled() && !preempt_disabled());

  char *cbuf = buf;
  int error = 0;

  assert(uio->uio_op == UIO_READ || uio->uio_op == UIO_WRITE);

  while (n > 0 && uio->uio_resid > 0) {
    /* Take the first io vector */
    iovec_t *iov = uio->uio_iov;
    size_t cnt = iov->iov_len;

    if (cnt == 0) {
      /* If no data left to move in this vector, proceed to the next io vector,
         or finish moving data if this was the last vector. */
      if (uio->uio_iovcnt == 0)
        break;
      uio->uio_iov++;
      uio->uio_iovcnt--;
      continue;
    }
    if (cnt > n)
      cnt = n;
    /* Perform copyout/copyin. */
    if (uio->uio_op == UIO_READ)
      error = copyout_vmspace(uio->uio_vmspace, cbuf, iov->iov_base, cnt);
    else
      error = copyin_vmspace(uio->uio_vmspace, iov->iov_base, cbuf, cnt);
    /* Exit immediately if there was a problem with moving data */
    if (error)
      break;

    /* Store progress on current io vector */
    iov->iov_base = (char *)iov->iov_base + cnt;
    iov->iov_len -= cnt;
    uio->uio_resid -= cnt;
    uio->uio_offset += cnt;
    cbuf += cnt;
    n -= cnt;
  }

  /* Invert error sign, because copy routines use negative error codes */
  return error;
}

int uiomove_frombuf(void *buf, size_t buflen, struct uio *uio) {
  size_t offset = uio->uio_offset;
  assert(offset <= buflen);
  assert(uio->uio_offset >= 0);

  return uiomove((char *)buf + offset, buflen - offset, uio);
}

int iovec_length(const iovec_t *iov, int iovcnt, size_t *lengthp) {
  size_t len = 0;
  for (int i = 0; i < iovcnt; i++) {
    len += iov[i].iov_len;
    /* Ensure that the total data size fits in ssize_t. */
    if (len > SSIZE_MAX || iov[i].iov_len > SSIZE_MAX)
      return EINVAL;
  }
  *lengthp = len;
  return 0;
}
