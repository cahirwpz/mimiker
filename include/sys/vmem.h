#ifndef _SYS_VMEM_H_
#define _SYS_VMEM_H_

#include <sys/types.h>

typedef uintptr_t vmem_addr_t;
typedef size_t vmem_size_t;

typedef struct vmem vmem_t;

/*
 * The following interface is a simplified version of NetBSD's vmem interface.
 * For documentation, please refer to VMEM(9), e.g. here:
 * https://netbsd.gw.com/cgi-bin/man-cgi?vmem+9+NetBSD-current
 */

vmem_t *vmem_create(const char *name, vmem_addr_t base, vmem_size_t size,
                    vmem_size_t quantum);
int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size);
int vmem_alloc(vmem_t *vm, vmem_size_t size, vmem_addr_t *addrp);
void vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size);
void vmem_destroy(vmem_t *vm);

#endif /* _SYS_VMEM_H_ */
