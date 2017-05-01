#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_

#include <vm.h>
#include <common.h>

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
  ssize_t uio_resid;     /* remaining bytes to process */
  uio_op_t uio_op;       /* operation */
  vm_map_t *uio_vmspace; /* destination address space */
} uio_t;

/* Convenience function for initializing uio, where iov has one buffer */
void prepare_single_uio(uio_t *uio, iovec_t *iov, uio_op_t op, vm_map_t *vm_map,
                        size_t offset, void *buffer, size_t buflen);

#define prepare_single_user_uio(uio, op, offset, buffer, buflen)               \
  iovec_t __CONCAT(iov_, __LINE__);                                            \
  prepare_single_uio((uio), &__CONCAT(iov_, __LINE__), (op),                   \
                     get_user_vm_map(), (offset), (buffer), (buflen));

#define prepare_single_kernel_uio(uio, op, offset, buffer, buflen)             \
  iovec_t __CONCAT(iov_, __LINE__);                                            \
  prepare_single_uio((uio), &__CONCAT(iov_, __LINE__), (op),                   \
                     get_kernel_vm_map(), (offset), (buffer), (buflen));

int uiomove(void *buf, size_t n, uio_t *uio);
int uiomove_frombuf(void *buf, size_t buflen, struct uio *uio);

#endif /* !_SYS_UIO_H_ */
