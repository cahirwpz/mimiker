#define KL_LOG KL_DEV
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/klog.h>
#include <sys/kmem.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/mimiker.h>
#include <sys/vm.h>
#include <libfdt/libfdt.h>

#define FDT_DEBUG 0

#define FDT_DEF_ADDR_CELLS 2
#define FDT_MAX_ADDR_CELLS 2

#define FDT_DEF_SIZE_CELLS 1
#define FDT_MAX_SIZE_CELLS 2

#define FDT_MAX_REG_CELLS (FDT_MAX_ADDR_CELLS + FDT_MAX_SIZE_CELLS)

#define FDT_DEF_INTR_CELLS 1

static void *fdtp; /* FDT blob virtual memory pointer */

static inline void FDT_perror(int err) {
#if defined(FDT_DEBUG) && FDT_DEBUG
  klog("FDT operation failed: %s", fdt_strerror(err));
#endif
}

static inline void FDT_panic(int err) {
  panic("FDT operation failed: %s", fdt_strerror(err));
}

void FDT_init(void *va) {
  int err = fdt_check_header(va);
  if (err < 0)
    FDT_panic(err);
  fdtp = va;
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
    /* There are no more subnodes. */
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

static void FDT_decode(pcell_t *cell, int cells) {
  for (int i = 0; i < cells; i++)
    cell[i] = be32toh(cell[i]);
}

void FDT_free(void *buf) {
  kfree(M_DEV, buf);
}

const char *FDT_getname(phandle_t node) {
  return fdt_get_name(fdtp, node, NULL);
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

  FDT_decode(buf, buflen / sizeof(pcell_t));

  return ret;
}

ssize_t FDT_getprop_alloc_multi(phandle_t node, const char *propname, int elsz,
                                void **bufp) {
  int len = FDT_getproplen(node, propname);
  if ((len == -1) || (len % elsz))
    return -1;

  void *buf = NULL;

  if (len) {
    buf = kmalloc(M_DEV, len, M_WAITOK);
    if (FDT_getprop(node, propname, buf, len) == -1) {
      FDT_free(buf);
      return -1;
    }
  }

  *bufp = buf;
  return len / elsz;
}

ssize_t FDT_getencprop_alloc_multi(phandle_t node, const char *propname,
                                   int elsz, void **bufp) {
  ssize_t rv = FDT_getprop_alloc_multi(node, propname, elsz, bufp);
  if (rv == -1)
    return -1;

  FDT_decode(*bufp, (rv * elsz) / sizeof(pcell_t));

  return rv;
}

ssize_t FDT_searchencprop(phandle_t node, const char *propname, pcell_t *buf,
                          size_t len) {
  for (; node != FDT_NODEV; node = FDT_parent(node)) {
    ssize_t rv = FDT_getencprop(node, propname, buf, len);
    if (rv != -1)
      return rv;
  }
  return -1;
}

int FDT_addrsize_cells(phandle_t node, int *addr_cellsp, int *size_cellsp) {
  const ssize_t cell_size = sizeof(pcell_t);
  pcell_t cell;

  /* Retrieve #address-cells. */
  if (FDT_getencprop(node, "#address-cells", &cell, cell_size) != cell_size)
    cell = FDT_DEF_ADDR_CELLS;
  *addr_cellsp = (int)cell;

  /* Retrieve #size-cells. */
  if (FDT_getencprop(node, "#size-cells", &cell, cell_size) != cell_size)
    cell = FDT_DEF_SIZE_CELLS;
  *size_cellsp = (int)cell;

  if (*addr_cellsp > FDT_MAX_ADDR_CELLS || *size_cellsp > FDT_MAX_SIZE_CELLS)
    return ERANGE;
  return 0;
}

int FDT_intr_cells(phandle_t node, int *intr_cellsp) {
  pcell_t icells;
  if (FDT_searchencprop(node, "#interrupt-cells", &icells, sizeof(pcell_t)) !=
      sizeof(pcell_t))
    icells = FDT_DEF_INTR_CELLS;
  if (icells < 1)
    return ERANGE;
  *intr_cellsp = icells;
  return 0;
}

u_long FDT_data_get(pcell_t *data, int cells) {
  if (cells == 1)
    return fdt32_to_cpu(*(uint32_t *)data);
  return fdt64_to_cpu(*(uint64_t *)data);
}

int FDT_data_to_res(pcell_t *data, int addr_cells, int size_cells,
                    u_long *addrp, u_long *sizep) {
  /* Address portion. */
  if (addr_cells > FDT_MAX_ADDR_CELLS)
    return ERANGE;
  *addrp = FDT_data_get(data, addr_cells);
  data += addr_cells;

  /* Size portion. */
  if (size_cells > FDT_MAX_SIZE_CELLS)
    return ERANGE;
  *sizep = FDT_data_get(data, size_cells);

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

  size_t cnt = 0;
  for (phandle_t child = FDT_child(rsv); child != FDT_NODEV;
       child = FDT_peer(child)) {
    if (cnt == FDT_MAX_RSV_MEM_REGS)
      return ERANGE;
    if (FDT_hasprop(child, "no-map"))
      continue;
    if (FDT_getprop(child, "reg", reg, sizeof(reg)) < 0)
      continue;
    FDT_data_to_res(reg, addr_cells, size_cells, &mrs[cnt].addr,
                    &mrs[cnt].size);
    cnt++;
  }
  *cntp = cnt;
  return 0;
}

int FDT_get_mem(fdt_mem_reg_t *mrs, size_t *cntp, size_t *sizep) {
  pcell_t reg[FDT_MAX_REG_CELLS * FDT_MAX_REG_TUPLES];

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

  if (!FDT_hasprop(chosen, "linux,initrd-start"))
    return ENXIO;
  if (!FDT_hasprop(chosen, "linux,initrd-end"))
    return ENXIO;

  pcell_t cell[2];
  u_long start, end;
  int cells;

  /* Retrieve start addr. */
  const size_t start_size =
    FDT_getencprop(chosen, "linux,initrd-start", cell, sizeof(cell));
  cells = start_size / sizeof(pcell_t);
  if (cells > FDT_MAX_ADDR_CELLS) {
    return ERANGE;
  } else if (cells == 1) {
    start = *(uint32_t *)cell;
  } else {
    uint64_t *startp = (uint64_t *)cell;
    start = *startp;
  }

  /* Retrieve end addr. */
  const size_t end_size =
    FDT_getencprop(chosen, "linux,initrd-end", cell, sizeof(cell));
  cells = end_size / sizeof(pcell_t);
  if (cells > FDT_MAX_ADDR_CELLS) {
    return ERANGE;
  } else if (cells == 1) {
    end = *(uint32_t *)cell;
  } else {
    uint64_t *endp = (uint64_t *)cell;
    end = *endp;
  }

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

int FDT_is_compatible(phandle_t node, const char *compatible) {
  int len;
  const void *prop = fdt_getprop(fdtp, node, "compatible", &len);
  if (!prop) {
    FDT_perror(len);
    return 0;
  }

  size_t inlen = strlen(compatible);
  while (len > 0) {
    size_t curlen = strlen(prop);
    if ((curlen == inlen) && !strncmp(prop, compatible, inlen))
      return 1;
    prop += curlen + 1;
    len -= curlen + 1;
  }
  return 0;
}
