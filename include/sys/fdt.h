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

void fdt_early_init(paddr_t pa, vaddr_t va);

void fdt_init(void);

void fdt_blob_range(paddr_t *startp, paddr_t *endp);

phandle_t fdt_finddevice(const char *device);

phandle_t fdt_child(phandle_t node);

phandle_t fdt_peer(phandle_t node);

ssize_t fdt_getptoplen(phandle_t node, const char *propname);

int fdt_hasprop(phandle_t node, const char *propname);

ssize_t fdt_getpropcpy(phandle_t node, const char *propname, pcell_t *buf,
                       size_t buflen);

int fdt_addrsize_cells(phandle_t node, int *addr_cells, int *size_cells);

u_long fdt_data_get(pcell_t *data, int cells);

int fdt_data_to_res(pcell_t *data, int addr_cells, int size_cells, u_long *addr,
                    u_long *size);

int fdt_get_reserved_mem(fdt_mem_reg_t *mrs, size_t *cntp);

int fdt_get_mem(fdt_mem_reg_t *mrs, size_t *cntp, size_t *sizep);

int fdt_get_chosen_initrd(fdt_mem_reg_t *mr);

int fdt_get_chosen_bootargs(const char **bootargsp);

#endif /* !_SYS_FDT_H_ */
