#ifndef _SYS_DEVICE_H_
#define _SYS_DEVICE_H_

#include <queue.h>
#include <uio.h>

typedef struct device device_t;

typedef int d_open_t(device_t *dev, int oflags);
typedef int d_close_t(device_t *dev);
typedef int d_read_t(device_t *dev, uio_t *uio, int ioflag);
typedef int d_write_t(device_t *dev, uio_t *uio, int ioflag);
typedef int d_mmap_t(device_t *dev, vm_offset_t offset, vm_paddr_t *paddr,
                     vm_size_t size, vm_prot_t prot);

struct device {
  LIST_ENTRY(device) d_list;
  unsigned d_flags;
  uint8_t d_major;
  uint8_t d_minor;
  const char *d_name;

  /* device switch table */
  d_open_t *d_open;
  d_close_t *d_close;
  d_read_t *d_read;
  d_write_t *d_write;
  d_mmap_t *d_mmap;
};

#endif /* _SYS_DEVICE_H_ */
