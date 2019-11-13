#include <sys/types.h>

#define VMEM_SLEEP 0x00000001
#define VMEM_NOSLEEP 0x00000002
#define VMEM_INSTANTFIT 0x00001000
#define VMEM_BESTFIT 0x00002000

typedef uintptr_t vmem_addr_t;
typedef size_t vmem_size_t;
typedef unsigned int vm_flag_t;

typedef struct vmem vmem_t;

vmem_t *vmem_create(const char *name, vmem_addr_t base, vmem_size_t size,
                    vmem_size_t quantum, vm_flag_t flags);

int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size, vm_flag_t flags);

int vmem_alloc(vmem_t *vm, vmem_size_t size, vm_flag_t flags,
               vmem_addr_t *addrp);

void vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size);

void vmem_destroy(vmem_t *vm);
