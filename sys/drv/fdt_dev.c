#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/libkern.h>

static int FDT_dev_soc_addr_to_cpu_addr(u_long soc_addr, u_long *cpu_addr_p) {
  phandle_t node = FDT_finddevice("/soc");
  if (node == FDT_NODEV)
    return ENXIO;

  int soc_addr_cells, soc_size_cells;
  int err = FDT_addrsize_cells(node, &soc_addr_cells, &soc_size_cells);
  if (err)
    return err;

  int cpu_addr_cells, cpu_size_cells;
  if ((err = FDT_addrsize_cells(0, &cpu_addr_cells, &cpu_size_cells)))
    return err;

  int tuple_cells = soc_addr_cells + cpu_addr_cells + soc_size_cells;
  size_t tuple_size = sizeof(pcell_t) * tuple_cells;
  pcell_t *tuples;

  ssize_t ntuples =
    FDT_getprop_alloc_multi(node, "ranges", tuple_size, (void **)&tuples);
  if (ntuples == -1)
    return ENXIO;

  if (!ntuples) {
    *cpu_addr_p = soc_addr;
    return 0;
  }

  for (int i = 0; i < ntuples; i++) {
    pcell_t *tuple = &tuples[i * tuple_cells];

    u_long range_soc_addr = FDT_data_get(tuple, soc_addr_cells);
    tuple += soc_addr_cells;

    u_long range_cpu_addr = FDT_data_get(tuple, cpu_addr_cells);
    tuple += cpu_addr_cells;

    u_long range_size = FDT_data_get(tuple, soc_size_cells);
    tuple += soc_size_cells;

    if (soc_addr >= range_soc_addr && soc_addr <= range_soc_addr + range_size) {
      u_long off = soc_addr - range_soc_addr;
      *cpu_addr_p = range_cpu_addr + off;
      goto end;
    }
  }
  err = EINVAL;

end:
  FDT_free(tuples);
  return err;
}

static int FDT_dev_get_region(device_t *dev, fdt_mem_reg_t *mrs, size_t *cntp) {
  phandle_t node = dev->node;

  int addr_cells, size_cells;
  int err = FDT_addrsize_cells(FDT_parent(node), &addr_cells, &size_cells);
  if (err)
    return err;

  int tuple_cells = addr_cells + size_cells;
  size_t tuple_size = sizeof(pcell_t) * tuple_cells;
  pcell_t *tuples;

  ssize_t ntuples =
    FDT_getprop_alloc_multi(node, "reg", tuple_size, (void **)&tuples);
  if (ntuples == -1)
    return ENXIO;

  if (ntuples > FDT_MAX_REG_TUPLES) {
    err = ERANGE;
    goto end;
  }

  for (int i = 0; i < ntuples; i++) {
    pcell_t *tuple = &tuples[i * tuple_cells];
    fdt_mem_reg_t *mr = &mrs[i];
    if ((err = FDT_data_to_res(tuple, addr_cells, size_cells, &mr->addr,
                               &mrs->size)))
      goto end;
    if ((err = FDT_dev_soc_addr_to_cpu_addr(mr->addr, &mr->addr)))
      goto end;
  }

end:
  *cntp = ntuples;
  FDT_free(tuples);
  return err;
}

int FDT_dev_get_intr_cells(device_t *dev, dev_intr_t *intr, pcell_t *cells,
                           size_t *cntp) {
  phandle_t node = dev->node;

  int icells;
  int err = FDT_intr_cells(intr->pic_id, &icells);
  if (err)
    return err;

  pcell_t *intrs;
  const char *propname =
    FDT_hasprop(node, "interrupts") ? "interrupts" : "interrupts-extended";
  ssize_t ncells = FDT_getencprop_alloc_multi(node, propname, sizeof(pcell_t),
                                              (void **)&intrs);
  if (ncells <= 0)
    return ENXIO;

  memcpy(cells, &intrs[intr->irq], icells * sizeof(pcell_t));

  *cntp = icells;
  FDT_free(intrs);
  return 0;
}

static int FDT_dev_region_to_rl(device_t *dev) {
  if (!FDT_hasprop(dev->node, "reg"))
    return 0;

  fdt_mem_reg_t *mrs = kmalloc(
    M_DEV, FDT_MAX_REG_TUPLES * sizeof(fdt_mem_reg_t), M_WAITOK | M_ZERO);

  size_t cnt;
  int err = FDT_dev_get_region(dev, mrs, &cnt);
  if (err)
    goto end;

  for (size_t i = 0; i < cnt; i++) {
    bus_addr_t start = mrs[i].addr;
    bus_addr_t end = start + mrs[i].size;
    device_add_mem(dev, i, start, end, 0);
  }

end:
  kfree(M_DEV, mrs);
  return err;
}

static int FDT_dev_intr_to_rl(device_t *dev) {
  phandle_t node = dev->node;
  phandle_t iparent = FDT_iparent(node);
  if (iparent == FDT_NODEV)
    return ENXIO;

  int icells;
  int err = FDT_intr_cells(iparent, &icells);
  if (err)
    return err;

  ssize_t intr_size = FDT_getproplen(node, "interrupts");
  if (intr_size < 0)
    return ENXIO;

  size_t tuple_size = sizeof(pcell_t) * icells;
  size_t nintrs = intr_size / tuple_size;

  for (size_t i = 0; i < nintrs; i++)
    device_add_intr(dev, i, iparent, i * icells);

  return 0;
}

static int FDT_dev_ext_intr_to_rl(device_t *dev) {
  phandle_t node = dev->node;

  pcell_t *cells;
  ssize_t ncells = FDT_getencprop_alloc_multi(node, "interrupts-extended",
                                              sizeof(pcell_t), (void **)&cells);
  if (ncells == -1)
    return ENXIO;
  if (!ncells)
    return 0;

  int err = 0;
  int icells;
  unsigned id = 0;
  for (int i = 0; i < ncells; i += icells, id++) {
    phandle_t iparent = FDT_finddevice_by_phandle(cells[i++]);
    if (iparent == FDT_NODEV)
      return ENXIO;

    if ((err = FDT_intr_cells(iparent, &icells)))
      goto end;
    if ((i + icells) > ncells) {
      err = ERANGE;
      goto end;
    }

    device_add_intr(dev, id, iparent, i);
  }

end:
  FDT_free(cells);
  return err;
}

int FDT_dev_add_child(device_t *bus, const char *path, device_bus_t dev_bus) {
  static int next_unit = 1;

  phandle_t node;
  if ((node = FDT_finddevice(path)) == FDT_NODEV)
    return ENXIO;

  device_t *dev = device_add_child(bus, next_unit);
  dev->bus = dev_bus;
  dev->node = node;

  int err;

  if ((err = FDT_dev_region_to_rl(dev)))
    goto bad;

  if (FDT_hasprop(node, "interrupts"))
    err = FDT_dev_intr_to_rl(dev);
  else if (FDT_hasprop(node, "interrupts-extended"))
    err = FDT_dev_ext_intr_to_rl(dev);

  if (err)
    goto bad;

  device_add_pending(dev);
  next_unit++;
  return 0;

bad:
  device_remove_child(bus, dev);
  return err;
}
