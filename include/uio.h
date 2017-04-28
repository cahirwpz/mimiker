#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_

#include <vm.h>

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

/* Uses -fplan9-extensions described in:
 * https://gcc.gnu.org/onlinedocs/gcc/Unnamed-Fields.html */
#define MAKE_UIO(name, op, vm_map, buf, count, offset)                         \
  struct {                                                                     \
    uio_t;                                                                     \
    iovec_t iov;                                                               \
  } name = {(uio_t){.uio_iov = &name.iov,                                      \
                    .uio_iovcnt = 1,                                           \
                    .uio_offset = (offset),                                    \
                    .uio_resid = (count),                                      \
                    .uio_op = (op),                                            \
                    .uio_vmspace = (vm_map)},                                  \
            (iovec_t){(buf), (count)}}

#define MAKE_UIO_USER(name, op, buf, count, offset)                            \
  MAKE_UIO(name, op, get_user_vm_map(), buf, count, offset)

#define MAKE_UIO_KERNEL(name, op, buf, count, offset)                          \
  MAKE_UIO(name, op, get_kernel_vm_map(), buf, count, offset)

int uiomove(void *buf, size_t n, uio_t *uio);
int uiomove_frombuf(void *buf, size_t buflen, struct uio *uio);

#endif /* !_SYS_UIO_H_ */
