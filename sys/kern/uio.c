#include <sys/klog.h>
#include <sys/uio.h>
#include <sys/libkern.h>
#include <sys/vm_map.h>
#include <sys/malloc.h>
#include <sys/errno.h>

typedef enum {
  UIOMOVE_NO_IO = 0x1,     /* Don't actually transfer any data */
  UIOMOVE_NO_MODIFY = 0x2, /* Don't modify uio structure */
} uiomove_flags_t;

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
static int _uiomove(void *buf, size_t n, uio_t *uio, uiomove_flags_t flags) {
  /* Calling uiomove from critical section (no interrupts or no preemption)
   * is not allowed since it may be copying from pageable memory. */
  assert(!intr_disabled() && !preempt_disabled());

  char *cbuf = buf;
  int error = 0;
  size_t done = 0;
  size_t iov_off = 0;
  int iov_idx = 0;

  assert(uio->uio_op == UIO_READ || uio->uio_op == UIO_WRITE);

  while (n > 0 && done < uio->uio_resid) {
    iovec_t *iov = &uio->uio_iov[iov_idx];
    size_t cnt = iov->iov_len - iov_off;

    if (cnt == 0) {
      /* If no data left to move in this vector, proceed to the next io vector.
       */
      assert(iov_idx < uio->uio_iovcnt);
      iov_idx++;
      iov_off = 0;
      continue;
    }
    if (cnt > n)
      cnt = n;

    if (!(flags & UIOMOVE_NO_IO)) {
      /* Perform copyout/copyin. */
      if (uio->uio_op == UIO_READ)
        error = copyout_vmspace(uio->uio_vmspace, cbuf, iov->iov_base, cnt);
      else
        error = copyin_vmspace(uio->uio_vmspace, iov->iov_base, cbuf, cnt);
      /* Exit immediately if there was a problem with moving data */
      if (error)
        break;

      cbuf += cnt;
    }

    iov_off += cnt;
    done += cnt;
    n -= cnt;
  }

  if (!(flags & UIOMOVE_NO_MODIFY)) {
    uio->uio_resid -= done;
    uio->uio_offset += done;
    uio->uio_iov += iov_idx;
    uio->uio_iovcnt -= iov_idx;
    if (uio->uio_iovcnt > 0) {
      uio->uio_iov->iov_base += iov_off;
      uio->uio_iov->iov_len -= iov_off;
    }
  }

  /* Invert error sign, because copy routines use negative error codes */
  return error;
}

int uiomove(void *buf, size_t n, uio_t *uio) {
  return _uiomove(buf, n, uio, 0);
}

int uio_peek(void *buf, size_t n, uio_t *uio) {
  if (uio->uio_op != UIO_WRITE)
    return EINVAL;
  return _uiomove(buf, n, uio, UIOMOVE_NO_MODIFY);
}

void uio_advance(uio_t *uio, size_t n) {
  _uiomove(NULL, n, uio, UIOMOVE_NO_IO);
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
