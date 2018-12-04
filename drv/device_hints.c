
#include <stdint.h>

typedef struct {
    char* path;
    uint64_t iomem_start;
    uint64_t iomem_end;
    uint32_t irq;
} devhint_t;

devhint_t hints[] = {
{ 	.iomem_start = 0x3f8,
	.irq = 0x4,
	.iomem_end = 0x3ff,
	.path = "/rootdev/pci@0/isab@0/isa@0/uart@0"
},
{ 	.iomem_start = 0x2f8,
	.irq = 0x3,
	.iomem_end = 0x2ff,
	.path = "/rootdev/pci@0/isab@0/isa@0/uart@1"
},

};

