/*
 * Based on the following FreeBSD files:
 *  - `sys/dev/fdt/fdt_common.c`,
 *  - `sys/dev/ofw/openfirm.c`,
 *  - `sys/dev/ofw/ofw_fdt.c`.
 *
 * More details regarding most presented functions can be found on appropriate
 * FreeBSD manual pages.
 */
#ifndef _SYS_FDT_H_
#define _SYS_FDT_H_

#include <sys/types.h>

#define FDT_MAX_RSV_MEM_REGS 16
#define FDT_MAX_REG_TUPLES 16
#define FDT_MAX_ICELLS 3
#define FDT_MAX_INTRS 16

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
 * FDT interrupt resource.
 */
typedef struct fdt_intr {
  pcell_t tuple[FDT_MAX_ICELLS];
  int icells;
  phandle_t iparent;
} fdt_intr_t;

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
 * Obtain the physical address of the FDT blob.
 */
paddr_t FDT_get_physaddr(void);

/*
 * Change the internal root FDT pointer.
 */
void FDT_changeroot(void *root);

/*
 * Obtain physical memory boundaries of the FDT blob.
 *
 * Arguments:
 *  - `startp`: dst for the first address
 *  - `endp`: dst for the first address after the blob
 */
void FDT_get_blob_range(paddr_t *startp, paddr_t *endp);

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
 * Returns:
 *  - `FDT_NODEV`: the node doesn't have any subnodes
 *  - otherwise: phandle of the first child
 */
phandle_t FDT_child(phandle_t node);

/*
 * Obtain the handle of the next sibling of device node `node`.
 *
 * `FDT_child` and `FDT_peer` are used to iterate over all children
 * of a given device node. Here is example code:
 *
 *   for (phandle_t child = FDT_child(rsv); child != FDT_NODEV;
 *     child = FDT_peer(child)) {
 *     ...
 *   }
 *
 * Returns:
 *  - `FDT_NODEV`: the node doesn't have any siblings
 *  - otherwise: phandle of the peer
 */
phandle_t FDT_peer(phandle_t node);

/*
 * Get the handle of the parent of device node `node`.
 *
 * Returns
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
ssize_t FDT_getproplen(phandle_t node, const char *propname);

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
 * `buflen` is specified in bytes not in cells.
 *
 * Returns:
 *  - >= 0: actual size of the property in bytes
 *  - -1: the property does not exist in the pointed node
 */
ssize_t FDT_getprop(phandle_t node, const char *propname, pcell_t *buf,
                    size_t buflen);

/*
 * The same as `FDT_getprop` but the copied cells are converted
 * from big-endian to host byte order.
 *
 * `buflen` must be divisible by 4 (`sizeof(pcell_t)`),
 * otherwise -1 is returned.
 */
ssize_t FDT_getencprop(phandle_t node, const char *propname, pcell_t *buf,
                       size_t buflen);

/*
 * Like `FDT_getencprop` but if `node` doesn't contain `propname`,
 * the function looks for its closest ancestor equipped with the property.
 */
ssize_t FDT_searchencprop(phandle_t node, const char *propname, pcell_t *buf,
                          size_t len);

/*
 * Copy the value of property `porpname` of device node `node`
 * into a newly allocated area returned via `bufp`.
 *
 * `propname` must be a property consisting of elements each of `elsz` bytes.
 *
 * Returns:
 *  - >= 0: number of elements composing `propname`
 *  - -1: the property does not exist or its length is not divisible by `elsz`
 */
ssize_t FDT_getprop_alloc_multi(phandle_t node, const char *propname, int elsz,
                                void **bufp);
/*
 * The same as `FDT_getprop_alloc_multi` but the copied cells are converted
 * from big-endian to host byte order.
 */
ssize_t FDT_getencprop_alloc_multi(phandle_t node, const char *propname,
                                   int elsz, void **bufp);

/*
 * Free memory allocated by an `FDT_*alloc*` function.
 */
void FDT_free(void *buf);

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
 * Obtain the "#interrupt-cells" property of device node `node`.
 *
 * Arguments:
 *  - `intr_cellsp`: dst for the property value
 *
 * Returns
 *  - 0: success
 *  - `ERANGE`: "#interrupt-cells" specifies a value
 *     that cannot be handled by the FDT module
 */
int FDT_intr_cells(phandle_t node, int *intr_cellsp);

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
 * The FDT module handles up to `FDT_MAX_REG_TUPLES` memory regions.
 * It is assumed that the length of provided memory region buffer `mrs`
 * is at least `FDT_MAX_REG_TUPLES`.
 *
 * Arguments:
 *  - `mrs`: FDT memory region buffer. This will be populated by the call.
 *  - `cntp`: The number of memory regions contained in the FDT.
 *  - `sizep`: Total size of all physical memory regions.
 *
 * Returns:
 *  - 0: success
 *  - `ENXIO`: the "/memory" device node could not be found or it does not
 *    contain a region property
 *  - `ERANGE`: the region property is too big or the totalsize of
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

/*
 * Check whether device node `node` is compatible
 * with device specified by `compatible`.
 *
 * All compatible strings of the device are examined.
 *
 * Return:
 *  - 0: no match
 *  - 1: match
 */
int FDT_is_compatible(phandle_t node, const char *compatible);

#endif /* !_SYS_FDT_H_ */
