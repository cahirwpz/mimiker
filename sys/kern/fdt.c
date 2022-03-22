#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/klog.h>
#include <sys/kmem.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/vm.h>
#include <libfdt/libfdt.h>

#define FDT_DEBUG 0

#define FDT_DEF_ADDR_CELLS 2
#define FDT_MAX_ADDR_CELLS 2

#define FDT_DEF_SIZE_CELLS 1
#define FDT_MAX_SIZE_CELLS 2

#define FDT_MAX_REG_CELLS (FDT_MAX_ADDR_CELLS + FDT_MAX_SIZE_CELLS)

static paddr_t fdt_pa;  /* FDT blob physical address */
static void *fdtp;      /* FDT blob virtual memory pointer */
static size_t fdt_size; /* FDT blob size (rounded to `PAGESIZE`) */

static inline void FDT_perror(int err) {
#if defined(FDT_DEBUG) && FDT_DEBUG
  klog("FDT operation failed: %s", fdt_strerror(err));
#endif
}

static inline void FDT_panic(int err) {
  panic("FDT operation failed: %s", fdt_strerror(err));
}

void FDT_early_init(paddr_t pa, vaddr_t va) {
  void *fdt = (void *)va;

  int err = fdt_check_header(fdt);
  if (err < 0)
    FDT_panic(err);

  const size_t totalsize = fdt_totalsize(fdt);

  fdt_pa = rounddown(pa, PAGESIZE);
  fdtp = fdt;
  fdt_size = roundup(fdt_pa + totalsize, PAGESIZE);
}

void FDT_init(void) {
  if (fdt_pa)
    fdtp = (void *)kmem_map_contig(fdt_pa, fdt_size, 0);
}

void FDT_blob_range(paddr_t *startp, paddr_t *endp) {
  *startp = fdt_pa;
  *endp = fdt_pa + fdt_size;
}

phandle_t FDT_finddevice(const char *device) {
  int node = fdt_path_offset(fdtp, device);
  if (node < 0) {
    FDT_perror(node);
    return FDT_NODEV;
  }
  return (phandle_t)node;
}

phandle_t FDT_child(phandle_t node) {
  int subnode = fdt_first_subnode(fdtp, node);
  if (subnode < 0) {
    /* The node doesn't have any subnodes. */
    return FDT_NODEV;
  }
  return (phandle_t)subnode;
}

phandle_t FDT_peer(phandle_t node) {
  int peer = fdt_next_subnode(fdtp, node);
  if (peer < 0) {
    /* There are no mode subnodes. */
    return FDT_NODEV;
  }
  return (phandle_t)peer;
}

phandle_t FDT_parent(phandle_t node) {
  int parent = fdt_parent_offset(fdtp, node);
  if (parent < 0) {
    FDT_perror(parent);
    return FDT_NODEV;
  }
  return (phandle_t)parent;
}

ssize_t FDT_getproplen(phandle_t node, const char *propname) {
  int len;
  const void *prop = fdt_getprop(fdtp, node, propname, &len);
  if (!prop) {
    FDT_perror(len);
    return -1;
  }
  return len;
}

int FDT_hasprop(phandle_t node, const char *propname) {
  return (FDT_getproplen(node, propname) >= 0) ? 1 : 0;
}

ssize_t FDT_getprop(phandle_t node, const char *propname, pcell_t *buf,
                    size_t buflen) {
  int len;
  const void *prop = fdt_getprop(fdtp, node, propname, &len);
  if (!prop) {
    FDT_perror(len);
    return -1;
  }
  memcpy(buf, prop, min((size_t)len, buflen));
  return len;
}

ssize_t FDT_getencprop(phandle_t node, const char *propname, pcell_t *buf,
                       size_t buflen) {
  if (buflen % 4)
    return -1;

  ssize_t ret = FDT_getprop(node, propname, buf, buflen);
  if (ret < 0)
    return -1;

  for (size_t i = 0; i < buflen / sizeof(uint32_t); i++)
    buf[i] = be32toh(buf[i]);

  return ret;
}

int FDT_addrsize_cells(phandle_t node, int *addr_cells, int *size_cells) {
  const ssize_t cell_size = sizeof(pcell_t);
  pcell_t cell;

  /* Retrieve #address-cells. */
  if (FDT_getencprop(node, "#address-cells", &cell, cell_size) < cell_size)
    cell = FDT_DEF_ADDR_CELLS;
  *addr_cells = (int)cell;

  /* Retrieve #size-cells. */
  if (FDT_getencprop(node, "#size-cells", &cell, cell_size) < cell_size)
    cell = FDT_DEF_SIZE_CELLS;
  *size_cells = (int)cell;

  if (*addr_cells > FDT_MAX_ADDR_CELLS || *size_cells > FDT_MAX_SIZE_CELLS)
    return ERANGE;
  return 0;
}

u_long FDT_data_get(pcell_t *data, int cells) {
  if (cells == 1)
    return fdt32_to_cpu(*(uint32_t *)data);
  return fdt64_to_cpu(*(uint64_t *)data);
}

int FDT_data_to_res(pcell_t *data, int addr_cells, int size_cells, u_long *addr,
                    u_long *size) {
  /* Address portion. */
  if (addr_cells > FDT_MAX_ADDR_CELLS)
    return ERANGE;
  *addr = FDT_data_get(data, addr_cells);
  data += addr_cells;

  /* Size portion. */
  if (size_cells > FDT_MAX_SIZE_CELLS)
    return ERANGE;
  *size = FDT_data_get(data, size_cells);

  return 0;
}

int FDT_get_reserved_mem(fdt_mem_reg_t *mrs, size_t *cntp) {
  pcell_t reg[FDT_MAX_REG_CELLS];

  phandle_t rsv = FDT_finddevice("/reserved-memory");
  if (rsv == FDT_NODEV)
    return ENXIO;

  int addr_cells, size_cells;
  int err = FDT_addrsize_cells(rsv, &addr_cells, &size_cells);
  if (err)
    return err;

  if (addr_cells + size_cells > FDT_MAX_REG_CELLS)
    return ERANGE;

  size_t cnt = 0;
  for (phandle_t child = FDT_child(rsv); child != FDT_NODEV;
       child = FDT_peer(child)) {
    if (cnt == FDT_MAX_MEM_REGS)
      return ERANGE;
    if (FDT_hasprop(child, "no-map"))
      continue;
    if (FDT_getprop(child, "reg", reg, FDT_MAX_REG_CELLS) < 0)
      continue;
    if ((err = FDT_data_to_res(reg, addr_cells, size_cells, &mrs[cnt].addr,
                               &mrs[cnt].size)))
      return err;
    cnt++;
  }
  *cntp = cnt;
  return 0;
}

int FDT_get_mem(fdt_mem_reg_t *mrs, size_t *cntp, size_t *sizep) {
  pcell_t reg[FDT_MAX_REG_CELLS * FDT_MAX_MEM_REGS];

  phandle_t mem = FDT_finddevice("/memory");
  if (mem == FDT_NODEV)
    return ENXIO;

  int addr_cells, size_cells;
  int err = FDT_addrsize_cells(FDT_parent(mem), &addr_cells, &size_cells);
  if (err)
    return err;

  const ssize_t reg_len = FDT_getproplen(mem, "reg");
  if (reg_len <= 0 || (size_t)reg_len > sizeof(reg))
    return ERANGE;

  if (FDT_getprop(mem, "reg", reg, reg_len) <= 0)
    return ENXIO;

  const int tuple_cells = addr_cells + size_cells;
  const size_t tuple_size = sizeof(pcell_t) * tuple_cells;
  const size_t ntuples = reg_len / tuple_size;
  size_t mem_size = 0;
  pcell_t *regp = reg;

  for (size_t i = 0; i < ntuples; i++) {
    if ((err = FDT_data_to_res(regp, addr_cells, size_cells, &mrs[i].addr,
                               &mrs[i].size)))
      return err;
    regp += tuple_cells;
    mem_size += mrs[i].size;
  }
  if (!mem_size)
    return ERANGE;

  *sizep = mem_size;
  *cntp = ntuples;
  return 0;
}

int FDT_get_chosen_initrd(fdt_mem_reg_t *mr) {
  phandle_t chosen = FDT_finddevice("/chosen");
  if (chosen == FDT_NODEV)
    return ENXIO;

  if (!FDT_hasprop(chosen, "linux,initrd-start") ||
      !FDT_hasprop(chosen, "linux,initrd-end"))
    return ENXIO;

  pcell_t cell[2];
  u_long start, end;
  int cells;

  /* Retrieve start addr. */
  const size_t start_size =
    FDT_getencprop(chosen, "linux,initrd-start", cell, sizeof(cell));
  if ((cells = start_size / sizeof(pcell_t)) > FDT_MAX_ADDR_CELLS)
    return ERANGE;
  if (cells == 1)
    start = *(uint32_t *)cell;
  else
    start = *(uint64_t *)cell;

  /* Retrieve end addr. */
  const size_t end_size =
    FDT_getencprop(chosen, "linux,initrd-end", cell, sizeof(cell));
  if ((cells = end_size / sizeof(pcell_t)) > FDT_MAX_ADDR_CELLS)
    return ERANGE;
  if (cells == 1)
    end = *(uint32_t *)cell;
  else
    end = *(uint64_t *)cell;

  *mr = (fdt_mem_reg_t){
    .addr = start,
    .size = end - start,
  };
  return 0;
}

int FDT_get_chosen_bootargs(const char **bootargsp) {
  phandle_t chosen = FDT_finddevice("/chosen");
  if (chosen == FDT_NODEV)
    return ENXIO;

  int len;
  const void *prop = fdt_getprop(fdtp, chosen, "bootargs", &len);
  if (!prop) {
    FDT_perror(len);
    return ENXIO;
  }
  *bootargsp = prop;
  return 0;
}
