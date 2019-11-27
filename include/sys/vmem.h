#ifndef _SYS_VMEM_H_
#define _SYS_VMEM_H_

#include <sys/types.h>
#include <sys/kmem_flags.h>

typedef uintptr_t vmem_addr_t;
typedef size_t vmem_size_t;
typedef struct vmem vmem_t;

/*
 * The following interface is a simplified version of NetBSD's vmem interface.
 * For detailed documentation, please refer to VMEM(9), e.g. here:
 * https://netbsd.gw.com/cgi-bin/man-cgi?vmem+9+NetBSD-current
 */

void vmem_bootstrap(void);

/*! \brief Create a new vmem arena.
 * You need to specify quantum, the smallest unit of allocation. */
vmem_t *vmem_create(const char *name, vmem_size_t quantum);

/*! \brief Add a new address span to the arena. */
int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size);

/*! \brief Allocate an address segment from the arena. */
int vmem_alloc(vmem_t *vm, vmem_size_t size, vmem_addr_t *addrp,
               kmem_flags_t flags);

/*! \brief Free segment previously allocated by vmem_alloc(). */
void vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size);

/*! \brief Destroy existing vmem arena. */
void vmem_destroy(vmem_t *vm);

#endif /* _SYS_VMEM_H_ */
