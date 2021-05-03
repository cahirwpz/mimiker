#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_

#include <sys/types.h>

typedef struct iovec {
  void *iov_base; /* Base address. */
  size_t iov_len; /* Length. */
} iovec_t;

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

#ifdef _KERNEL

#include <sys/vm.h>

typedef struct vm_map vm_map_t;

/* This structure stores just enough information about an uio
 * to be able to restore it after the uio is modified by uiomove(). */
typedef struct uiostate {
  size_t us_resid;  /* Saved value of uio_resid */
  int us_iovcnt;    /* Saved value of uio_iovcnt */
  size_t us_iovoff; /* Saved value of uio_iovoff */
} uiostate_t;

typedef enum { UIO_READ, UIO_WRITE } uio_op_t;

typedef struct uio {
  iovec_t *uio_iov;      /* scatter/gather list */
  int uio_iovcnt;        /* length of scatter/gather list */
  size_t uio_iovoff;     /* offset in current iov segment */
  off_t uio_offset;      /* offset in target object */
  size_t uio_resid;      /* remaining bytes to process */
  uio_op_t uio_op;       /* operation */
  vm_map_t *uio_vmspace; /* destination address space */
  unsigned uio_ioflags;  /* IO_* flags associated with this operation */
} uio_t;

#define UIO_SINGLE(op, vm_map, offset, buf, buflen)                            \
  (uio_t) {                                                                    \
    .uio_iov = (iovec_t[1]){(iovec_t){__UNCONST(buf), (buflen)}},              \
    .uio_iovcnt = 1, .uio_iovoff = 0, .uio_offset = (offset),                  \
    .uio_resid = (buflen), .uio_op = (op), .uio_vmspace = (vm_map)             \
  }

#define UIO_SINGLE_KERNEL(op, offset, buf, buflen)                             \
  UIO_SINGLE(op, vm_map_kernel(), offset, buf, buflen)

#define UIO_SINGLE_USER(op, offset, buf, buflen)                               \
  UIO_SINGLE(op, vm_map_user(), offset, buf, buflen)

#define UIO_VECTOR(op, vm_map, iov, iovcnt, len)                               \
  (uio_t) {                                                                    \
    .uio_iov = (iov), .uio_iovcnt = (iovcnt), .uio_iovoff = 0,                 \
    .uio_offset = 0, .uio_resid = (len), .uio_op = (op),                       \
    .uio_vmspace = (vm_map)                                                    \
  }

#define UIO_VECTOR_KERNEL(op, iov, iovcnt, len)                                \
  UIO_VECTOR(op, vm_map_kernel(), iov, iovcnt, len)

#define UIO_VECTOR_USER(op, iov, iovcnt, len)                                  \
  UIO_VECTOR(op, vm_map_user(), iov, iovcnt, len)

int uiomove(void *buf, size_t n, uio_t *uio);
void uio_save(const uio_t *uio, uiostate_t *save);
void uio_restore(uio_t *uio, const uiostate_t *save);
int uiomove_frombuf(void *buf, size_t buflen, struct uio *uio);
int iovec_length(const iovec_t *iov, int iovcnt, size_t *lengthp);

#endif /* _KERNEL */

#endif /* !_SYS_UIO_H_ */
