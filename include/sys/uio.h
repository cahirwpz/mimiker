#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_

#include <sys/vm.h>

typedef struct vm_map vm_map_t;

typedef struct iovec {
  void *iov_base; /* Base address. */
  size_t iov_len; /* Length. */
} iovec_t;

typedef enum { UIO_READ, UIO_WRITE } uio_op_t;

typedef struct uio {
  iovec_t *uio_iov;      /* scatter/gather list */
  int uio_iovcnt;        /* length of scatter/gather list */
  off_t uio_offset;      /* offset in target object */
  size_t uio_resid;      /* remaining bytes to process */
  size_t uio_iov_off;    /* offset in current iovec segment */
  uio_op_t uio_op;       /* operation */
  vm_map_t *uio_vmspace; /* destination address space */
} uio_t;

#define UIO_SINGLE(op, vm_map, offset, buf, buflen)                            \
  (uio_t) {                                                                    \
    .uio_iov = (iovec_t[1]){(iovec_t){__UNCONST(buf), (buflen)}},              \
    .uio_iovcnt = 1, .uio_offset = (offset), .uio_resid = (buflen),            \
    .uio_iov_off = 0, .uio_op = (op), .uio_vmspace = (vm_map)                  \
  }

#define UIO_SINGLE_KERNEL(op, offset, buf, buflen)                             \
  UIO_SINGLE(op, vm_map_kernel(), offset, buf, buflen)

#define UIO_SINGLE_USER(op, offset, buf, buflen)                               \
  UIO_SINGLE(op, vm_map_user(), offset, buf, buflen)

int uiomove(void *buf, size_t n, uio_t *uio);
int uiomove_frombuf(void *buf, size_t buflen, struct uio *uio);

#endif /* !_SYS_UIO_H_ */
