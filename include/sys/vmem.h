#include <sys/types.h>

#define	VM_SLEEP	0x00000001
#define	VM_NOSLEEP	0x00000002
#define	VM_INSTANTFIT	0x00001000

typedef	uintptr_t vmem_addr_t;
typedef size_t vmem_size_t;
typedef unsigned int vm_flag_t;

#define	VMEM_ADDR_MIN	0
#define	VMEM_ADDR_MAX	(~(vmem_addr_t)0)

typedef struct vmem vmem_t;
typedef struct vmem_btag vmem_btag_t;
