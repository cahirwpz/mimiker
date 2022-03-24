#ifndef _SYS_FDT_H_
#define _SYS_FDT_H_

#include <sys/types.h>

#define FDT_MAX_RSV_MEM_REGS 16
#define FDT_MAX_MEM_REGS 16

typedef uint32_t phandle_t;
typedef uint32_t pcell_t;

#define FDT_NODEV ((phandle_t)-1)

/*
 * FDT memory region.
 */
typedef struct fdt_mem_reg {
  u_long addr;
  u_long size;
} fdt_mem_reg_t;

/*
 * Early FDT initialization.
 *
 * Must be called during MD bootstrap before the kernel environment is built.
 * This is the first FDT function that gets called.
 *
 * Arguments:
 *  - `pa`: FDT physical address
 *  - `va`: FDT kernel virtual address
 */
void FDT_early_init(paddr_t pa, vaddr_t va);

/*
 * Final FDT initialization.
 *
 * Called during MI kernel bootstrap.
 */
void FDT_init(void);

/*
 * Obtain physical memory boundaries of the FDT blob.
 *
 * Arguments:
 *  - `startp`: dst for the first address
 *  - `endp`: dst for the first address after the blob
 */
void FDT_blob_range(paddr_t *startp, paddr_t *endp);

/*
 * Find the package handle of a pointed device in the device tree.
 *
 * Arguments:
 *  - `device`: device path
 *
 * Returns:
 *  - `FDT_NODEV`: the path could not be found
 *  - otherwise: phandle of the requested device
 */
phandle_t FDT_finddevice(const char *device);

/*
 * Obtain the handle of the first child of device node `node`.
 *
 *  Returns:
 *  - `FDT_NODEV`: the node doesn't have any subnodes
 *  - otherwise: phandle of the first child
 */
phandle_t FDT_child(phandle_t node);

/*
 * Obtain the handle of the next sibling of device node `node`.
 *
 *  Returns:
 *  - `FDT_NODEV`: the node doesn't have any siblings
 *  - otherwise: phandle of the peer
 */
phandle_t FDT_peer(phandle_t node);

/*
 * Get the handle of the parent of device node `node`.
 *
 * Error codes:
 *  - `FDT_NODEV`: the pointed node or FDT state is invalid
 *  - 0: `node` is the root node
 *  - otherwise: phandle of the parent node
 */
phandle_t FDT_parent(phandle_t node);

/*
 * Obtain the length of the value associated with property `propname`
 * in node `node`.
 *
 * Returns:
 *  - >= 0: the actual length
 *  - -1: the property does not exist in the pointed node
 */
ssize_t FDT_getptoplen(phandle_t node, const char *propname);

/*
 * Verify if node `node` has property specified by `propname`.
 *
 * Returns:
 *  - 1: the device node has the `propname` property
 *  - 0: the property does not exist in the pointed node
 */
int FDT_hasprop(phandle_t node, const char *propname);

/*
 * Copy maximum of `buflen` bytes from the value associated with the property
 * `propname` of the device node `node` into the memory specified by `buf`.
 *
 * Returns:
 *  - >= 0: actual size of the property
 *  - -1: the property does not exist in the pointed node
 */
ssize_t FDT_getprop(phandle_t node, const char *propname, pcell_t *buf,
                    size_t buflen);

/*
 * The same as `FDT_getprop` but the copied cells are converted
 * from big-endian to host byte order.
 */
ssize_t FDT_getencprop(phandle_t node, const char *propname, pcell_t *buf,
                       size_t buflen);

/*
 * Obtain the "#address-cells" and "#size-cells" properties
 * of device node `node`.
 *
 * Arguments:
 *  - `addr_cellsp`: dst of the "#address-cells" property value
 *  - `size_cellsp`: dst of the "#size-cells" property value
 *
 * Returns
 *  - 0: success
 *  - `ERANGE`: either "#address-cells" or "#size-cells" specifies a value
 *     that cannot be handled by the FDT module
 */
int FDT_addrsize_cells(phandle_t node, int *addr_cellsp, int *size_cellsp);

/*
 * Convert a 32- or 64-bit value contained in cell buffer `data`
 * into an unsigned long host value.
 *
 * Arguments:
 *  - `cells`: specified the size of the value contained in `data`
 *     (`sizeof(uint32_t)` or `sizeof(uint64_t)`)
 */
u_long FDT_data_get(pcell_t *data, int cells);

/*
 * Convert an (addr, size) property contained in `data` to host unsigned long
 * values.
 *
 * Arguments:
 *  - `addr_cells`: size of the address portion
 *  - `size_cells`: size of the size portion
 *  - `addrp`: dst for the converted address
 *  - `sizep`: dst for the converted size
 *
 * Returns:
 *  - 0: success
 *  - `ERANGE`: the property is too big to be handled by the FDT module
 */
int FDT_data_to_res(pcell_t *data, int addr_cells, int size_cells,
                    u_long *addrp, u_long *sizep);

/*
 * Obtain reserved memory regions of the FDT.
 *
 * The FDT module handles up to `FDT_MAX_RSV_MEM_REGS` reserved memory regions.
 * It is assumed that the length of provided memory region buffer `mrs`
 * is at least `FDT_MAX_RSV_MEM_REGS`.
 *
 * Arguments:
 *  - `mrs`: FDT memory region buffer. This will be populated by the call.
 *  - `cntp`: The number of reserved memory regions contained in the FDT.
 *
 * Returns:
 *  - 0: success
 *  - `ENXIO`: the "/reserved-memory" device node could not be found
 *  - `ERANGE`: subnodes of the "/reserved-memory" node are too big
 *    to be handled by the FDT module, or there is too much of them
 */
int FDT_get_reserved_mem(fdt_mem_reg_t *mrs, size_t *cntp);

/*
 * Obtain physical memory regions of the FDT.
 *
 * The FDT module handles up to `FDT_MAX_MEM_REGS` reserved memory regions.
 * It is assumed that the length of provided memory region buffer `mrs`
 * is at least `FDT_MAX_MEM_REGS`.
 *
 * Arguments:
 *  - `mrs`: FDT memory region buffer. This will be populated by the call.
 *  - `cntp`: The number of memory regions contained in the FDT.
 *  - `sizep`: Total size of all physical memory regions.
 *
 * Returns:
 *  - 0: success
 *  - `ENXIO`: the "/memory" device node could not be found or it does not
 *    contend a region property
 *  - `ERANGE`: the region property is too big or  the totalsize of
 *    physical memory is zero
 */
int FDT_get_mem(fdt_mem_reg_t *mrs, size_t *cntp, size_t *sizep);

/*
 * Obtain the boundaries of the initial ramdisk specified in the "/chosen"
 * device node.
 *
 * Arguments:
 *  - `mr`: FDT memory region to fill by the call
 *
 * Returns:
 *  - 0: success
 *  - `ENXIO`: the "/chosen" node could not be found or the initrd boundaries
 *    are not included in the node
 *  - `ERANGE`: either start address of end address property is too big
 *    to be handled by the FDT module
 */
int FDT_get_chosen_initrd(fdt_mem_reg_t *mr);

/*
 * Obtain the bootargs property of the "/chosen" node.
 *
 * Arguments:
 *  - `bootargsp`: dst for the address of the bootargs property in the FDT blob
 *
 * Returns:
 *  - 0: success
 *  - `ENXIO`: the "/chosen" node could not be found or the bootargs property
 *    is not included
 */
int FDT_get_chosen_bootargs(const char **bootargsp);

#endif /* !_SYS_FDT_H_ */
