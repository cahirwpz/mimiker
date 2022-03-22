#ifndef _SYS_FDT_H_
#define _SYS_FDT_H_

#include <sys/types.h>

#define FDT_MAX_RSV_MEM_REGS 16
#define FDT_MAX_MEM_REGS 16

typedef uint32_t phandle_t;
typedef uint32_t pcell_t;

#define FDT_NODEV ((phandle_t)-1)

typedef struct fdt_mem_reg {
  u_long addr;
  u_long size;
} fdt_mem_reg_t;

void FDT_early_init(paddr_t pa, vaddr_t va);

void FDT_init(void);

void FDT_blob_range(paddr_t *startp, paddr_t *endp);

phandle_t FDT_finddevice(const char *device);

phandle_t FDT_child(phandle_t node);

phandle_t FDT_peer(phandle_t node);

ssize_t FDT_getptoplen(phandle_t node, const char *propname);

int FDT_hasprop(phandle_t node, const char *propname);

ssize_t FDT_getprop(phandle_t node, const char *propname, pcell_t *buf,
                    size_t buflen);

int FDT_addrsize_cells(phandle_t node, int *addr_cells, int *size_cells);

u_long FDT_data_get(pcell_t *data, int cells);

int FDT_data_to_res(pcell_t *data, int addr_cells, int size_cells, u_long *addr,
                    u_long *size);

int FDT_get_reserved_mem(fdt_mem_reg_t *mrs, size_t *cntp);

int FDT_get_mem(fdt_mem_reg_t *mrs, size_t *cntp, size_t *sizep);

int FDT_get_chosen_initrd(fdt_mem_reg_t *mr);

int FDT_get_chosen_bootargs(const char **bootargsp);

#endif /* !_SYS_FDT_H_ */
