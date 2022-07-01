#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <dev/fdt_dev.h>

int FDT_dev_is_compatible(device_t *dev, const char *compatible) {
  return FDT_is_compatible(dev->node, compatible);
}

static bool FDT_dev_is_simplebus(device_t *dev) {
  return dev->node == FDT_finddevice("/soc");
}

static device_t *FDT_dev_get_rootdev(device_t *dev) {
  while (dev->parent)
    dev = dev->parent;
  return dev;
}

static device_t *FDT_dev_get_simplebus(device_t *rootdev) {
  device_t *dev;
  TAILQ_FOREACH (dev, &rootdev->children, link) {
    if (FDT_dev_is_simplebus(dev))
      return dev;
  }
  return NULL;
}

static device_t *FDT_dev_new_device(device_t *bus, phandle_t node, int unit) {
  device_t *dev = device_add_child(bus, unit);
  dev->node = node;

  if (FDT_dev_is_simplebus(bus))
    dev->bus = DEV_BUS_SIMPLEBUS;

  dev->phandle = FDT_NODEV;

  if (FDT_getencprop(node, "phandle", &dev->phandle, sizeof(pcell_t)) ==
      sizeof(pcell_t))
    return dev;

  /* We assume that each interrupt controller has a `phandle` property. */
  if (FDT_hasprop(node, "interrupt-controller")) {
    device_remove_child(bus, dev);
    return NULL;
  }

  return dev;
}

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

int FDT_dev_get_region(device_t *dev, fdt_mem_reg_t *mrs, size_t *cntp) {
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
    if (!FDT_dev_is_simplebus(dev->parent))
      continue;
    if ((err = FDT_dev_soc_addr_to_cpu_addr(mr->addr, &mr->addr)))
      goto end;
  }

end:
  *cntp = ntuples;
  FDT_free(tuples);
  return err;
}

static int FDT_dev_get_interrupts(device_t *dev, fdt_intr_t *intrs,
                                  size_t *cntp) {
  phandle_t node = dev->node;
  phandle_t iparent = dev->pic->node;

  int icells;
  int err = FDT_intr_cells(iparent, &icells);
  if (err)
    return err;

  size_t tuple_size = sizeof(pcell_t) * icells;
  pcell_t *tuples;

  int ntuples = FDT_getencprop_alloc_multi(node, "interrupts", tuple_size,
                                           (void **)&tuples);
  if (ntuples == -1)
    return ENXIO;

  if (!ntuples)
    return 0;

  if (ntuples > FDT_MAX_INTRS) {
    err = ERANGE;
    goto end;
  }

  for (int i = 0; i < ntuples * icells; i += icells, intrs++) {
    memcpy(intrs->tuple, &tuples[i], tuple_size);
    intrs->icells = icells;
    intrs->iparent = iparent;
  }

end:
  *cntp = ntuples;
  FDT_free(tuples);
  return err;
}

static int FDT_dev_get_interrupts_extended(device_t *dev, fdt_intr_t *intrs,
                                           size_t *cntp) {
  phandle_t node = dev->node;

  pcell_t *cells;
  int ncells = FDT_getencprop_alloc_multi(node, "interrupts-extended",
                                          sizeof(pcell_t), (void **)&cells);
  if (ncells == -1)
    return ENXIO;
  if (!ncells)
    return 0;

  size_t ntuples = 0;
  int err = 0, icells;
  for (int i = 0; i < ncells; i += icells, intrs++, ntuples++) {
    if (ntuples > FDT_MAX_INTRS) {
      err = ERANGE;
      goto end;
    }

    /* TODO: at this point we should find the interrupt parent
     * based on the phandle encoded in the current tuple.
     * FTTB, we assume that each device has at most one interrupt controller. */
    phandle_t iparent = dev->pic->node;
    i++;

    if ((err = FDT_intr_cells(iparent, &icells)))
      goto end;
    if ((i + icells) > ncells) {
      err = ERANGE;
      goto end;
    }

    size_t tuple_size = sizeof(pcell_t) * icells;
    memcpy(intrs->tuple, &cells[i], tuple_size);
    intrs->icells = icells;
    intrs->iparent = iparent;
  }

end:
  *cntp = ntuples;
  FDT_free(cells);
  return err;
}

static int FDT_dev_region_to_rl(device_t *dev) {
  phandle_t node = dev->node;

  if (!FDT_hasprop(node, "reg"))
    return 0;

  fdt_mem_reg_t *mrs = kmalloc(
    M_DEV, FDT_MAX_REG_TUPLES * sizeof(fdt_mem_reg_t), M_WAITOK | M_ZERO);

  size_t cnt;
  int err = FDT_dev_get_region(dev, mrs, &cnt);
  if (err)
    return err;

  for (size_t i = 0; i < cnt; i++)
    device_add_memory(dev, i, mrs[i].addr, mrs[i].size);

  kfree(M_DEV, mrs);
  return 0;
}

static int FDT_dev_intr_to_rl(device_t *dev) {
  phandle_t node = dev->node;

  if (!FDT_hasprop(node, "interrupts") &&
      !FDT_hasprop(node, "interrupts-extended"))
    return 0;

  device_t *pic = dev->pic;
  assert(pic);
  assert(pic->driver);

  fdt_intr_t *intrs =
    kmalloc(M_DEV, FDT_MAX_INTRS * sizeof(fdt_intr_t), M_WAITOK | M_ZERO);
  size_t nintrs;
  int err = 0;

  if (FDT_hasprop(node, "interrupts"))
    err = FDT_dev_get_interrupts(dev, intrs, &nintrs);
  else
    err = FDT_dev_get_interrupts_extended(dev, intrs, &nintrs);

  if (err)
    goto end;

  int rid = 0;
  for (size_t i = 0; i < nintrs; i++) {
    int irqnum = pic_map_intr(dev, &intrs[i]);
    if (irqnum >= 0)
      device_add_irq(dev, rid++, irqnum);
  }

end:
  kfree(M_DEV, intrs);
  return err;
}

static device_t *FDT_dev_find_pic(device_t *dev, device_t *rootdev) {
  phandle_t node = dev->node;

  pcell_t iparent_phandle = FDT_get_iparent_phandle(node);
  if (iparent_phandle == FDT_NODEV)
    return NULL;

  if (iparent_phandle == rootdev->phandle)
    return rootdev;

  device_t *sb = FDT_dev_get_simplebus(rootdev);
  if (!sb)
    return NULL;

  device_t *pic;
  TAILQ_FOREACH (pic, &sb->children, link) {
    if (iparent_phandle == pic->phandle)
      return pic;
  }
  return NULL;
}

int FDT_dev_add_device_by_node(device_t *bus, phandle_t node, int unit) {
  int err = 0;

  device_t *dev = FDT_dev_new_device(bus, node, unit);
  if (!dev)
    return ENXIO;

  device_t *rootdev = FDT_dev_get_rootdev(bus);

  if (FDT_hasprop(node, "interrupts") ||
      FDT_hasprop(node, "interrupts-extended")) {
    dev->pic = FDT_dev_find_pic(dev, rootdev);
    if (!dev->pic) {
      err = ENXIO;
      goto end;
    }
  }

  bool is_simplebus = FDT_dev_region_to_rl(dev);

  if (!is_simplebus) {
    if ((err = FDT_dev_region_to_rl(dev)))
      goto end;
    if ((err = FDT_dev_intr_to_rl(dev)))
      goto end;
  }

  if (is_simplebus || FDT_hasprop(node, "interrupt-controller")) {
    err = bus_generic_probe(bus);
  }

end:
  if (err)
    device_remove_child(rootdev, dev);
  return err;
}

int FDT_dev_add_device_by_path(device_t *bus, const char *path, int unit) {
  phandle_t node;
  if ((node = FDT_finddevice(path)) == FDT_NODEV)
    return ENXIO;

  return FDT_dev_add_device_by_node(bus, node, unit);
}
