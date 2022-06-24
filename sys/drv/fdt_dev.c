#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/libkern.h>

static device_list_t sb_pending_ics = TAILQ_HEAD_INITIALIZER(sb_pending_ics);
static int sb_unit;
static phandle_t sb_node;
static int sb_soc_addr_cells, sb_soc_size_cells;
static int sb_cpu_addr_cells, sb_cpu_size_cells;

static device_t *sb_new_child(phandle_t node, int unit) {
  device_t *dev = device_alloc(unit);
  dev->bus = DEV_BUS_SIMPLEBUS;
  dev->node = node;
  if (FDT_getencprop(node, "phandle", &dev->handle,
                     sizeof(pcell_t)) != sizeof(pcell_t)) {
    if (FDT_hasprop(node, "interrupt-controller"))
      panic("Interrupt controller without a phandle property!");
    return NULL;
  }
  return dev;
}

static void sb_remove_pending_ic(device_t *ic) {
  assert(ic->bus == DEV_BUS_SIMPLEBUS);
  TAILQ_REMOVE(&sb_pending_ics, ic, link);
  kfree(M_DEV, ic);
}

static int sb_soc_addr_to_cpu_addr(u_long soc_addr, u_long *cpu_addr_p) {
  int tuple_cells = sb_soc_addr_cells + sb_cpu_addr_cells + sb_soc_size_cells;
  size_t tuple_size = sizeof(pcell_t) * tuple_cells;
  pcell_t *tuples;

  ssize_t ntuples =
    FDT_getprop_alloc_multi(sb_node, "ranges", tuple_size, (void **)&tuples);
  if (ntuples == -1)
    return ENXIO;

  if (!ntuples) {
    *cpu_addr_p = soc_addr;
    return 0;
  }

  int err = 0;

  for (int i = 0; i < ntuples; i++) {
    pcell_t *tuple = &tuples[i * tuple_cells];

    u_long range_soc_addr = FDT_data_get(tuple, sb_soc_addr_cells);
    tuple += sb_soc_addr_cells;

    u_long range_cpu_addr = FDT_data_get(tuple, sb_cpu_addr_cells);
    tuple += sb_cpu_addr_cells;

    u_long range_size = FDT_data_get(tuple, sb_soc_size_cells);
    tuple += sb_soc_size_cells;

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

static int sb_get_region(phandle_t node, fdt_mem_reg_t *mrs, size_t *cntp) {
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
    if ((err = sb_soc_addr_to_cpu_addr(mr->addr, &mr->addr)))
      goto end;
  }

end:
  *cntp = ntuples;
  FDT_free(tuples);
  return err;
}

static int sb_get_interrupts(device_t *dev, fdt_intr_t *intrs, size_t *cntp) {
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

static int sb_get_interrupts_extended(device_t *dev, fdt_intr_t *intrs,
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

static int sb_region_to_rl(device_t *dev) {
  if (!FDT_hasprop(dev->node, "reg"))
    return 0;

  fdt_mem_reg_t *mrs = kmalloc(
    M_DEV, FDT_MAX_REG_TUPLES * sizeof(fdt_mem_reg_t), M_WAITOK | M_ZERO);

  size_t cnt;
  int err = sb_get_region(dev->node, mrs, &cnt);
  if (err)
    return err;

  for (size_t i = 0; i < cnt; i++)
    device_add_memory(dev, i, mrs[i].addr, mrs[i].size);

  kfree(M_DEV, mrs);
  return 0;
}

static int sb_intr_to_rl(device_t *dev) {
  device_t *pic = dev->pic;
  assert(pic);
  assert(pic->driver);

  fdt_intr_t *intrs =
    kmalloc(M_DEV, FDT_MAX_INTRS * sizeof(fdt_intr_t), M_WAITOK | M_ZERO);
  phandle_t node = dev->node;
  size_t nintrs;
  int err = 0;

  if (FDT_hasprop(node, "interrupts"))
    err = sb_get_interrupts(dev, intrs, &nintrs);
  else if (FDT_hasprop(node, "interrupts-extended"))
    err = sb_get_interrupts_extended(dev, intrs, &nintrs);
  else
    goto end;

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

static int sb_discover_io_map(rman_addr_t *startp, rman_addr_t *endp) {
  fdt_mem_reg_t *mrs = kmalloc(
    M_DEV, FDT_MAX_REG_TUPLES * sizeof(fdt_mem_reg_t), M_WAITOK | M_ZERO);

  rman_addr_t start = RMAN_ADDR_MAX;
  rman_addr_t end = 0;
  int err = 0;

  for (phandle_t node = FDT_child(sb_node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    size_t cnt;
    if ((err = sb_get_region(node, mrs, &cnt)))
      goto end;

    for (size_t i = 0; i < cnt; i++) {
      u_long reg_start = mrs[i].addr;
      u_long reg_end = mrs[i].addr + mrs[i].size;
      assert(reg_start < reg_end);
      if (reg_start < start)
        start = reg_start;
      if (reg_end > end)
        end = reg_end;
    }
  }

  assert((start != RMAN_ADDR_MAX) && end && (start < end));
  *startp = start;
  *endp = end;

end:
  kfree(M_DEV, mrs);
  return err;
}

static phandle_t sb_find_iparent(phandle_t node) {
  phandle_t iparent;

  /* XXX: FTTB, we assume that each device
   * has at most one interrupt controller. */
  if (FDT_getencprop(node, "interrupts-extended", &iparent,
                     sizeof(phandle_t)) >= (ssize_t)sizeof(phandle_t))
    return iparent;

  if (FDT_searchencprop(node, "interrupt-parent", &iparent,
                        sizeof(phandle_t)) == sizeof(phandle_t))
    return iparent;

  for (; node != FDT_NODEV; node = FDT_peer(node))
    if (FDT_hasprop(node, "interrupt-controller"))
      break;
  return node;
}

static device_t *sb_find_pic(device_t *rootdev, phandle_t node) {
  phandle_t iparent = sb_find_iparent(node);
  if (iparent == FDT_NODEV)
    return NULL;

  if (iparent == rootdev->node)
    return rootdev;

  device_list_t *list = &sb_pending_ics;
  if (TAILQ_EMPTY(list))
    list = &rootdev->children;

  device_t *dev;
  TAILQ_FOREACH (dev, list, link) {
    if (dev->node == iparent)
      return dev;
  }
  return NULL;
}

static int sb_discover_ics(device_t *rootdev) {
  int nics = 0;

  for (phandle_t node = FDT_child(sb_node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    if (!FDT_hasprop(node, "interrupt-controller"))
      continue;
    device_t *dev = sb_new_child(node, nics++);
    TAILQ_INSERT_TAIL(&sb_pending_ics, dev, link);
  }

  sb_unit = nics;

  device_t *ic;
  TAILQ_FOREACH (ic, &sb_pending_ics, link) {
    ic->pic = sb_find_pic(rootdev, ic->node);
    if (!ic->pic)
      return ENXIO;
  }

  int err = 0;

  while (!TAILQ_EMPTY(&sb_pending_ics)) {
    device_t *next;
    TAILQ_FOREACH_SAFE (ic, &sb_pending_ics, link, next) {
      if (!ic->pic->driver)
        continue;

      TAILQ_REMOVE(&sb_pending_ics, ic, link);
      TAILQ_INSERT_TAIL(&rootdev->children, ic, link);

      if ((err = sb_region_to_rl(ic)))
        return err;
      if ((err = sb_intr_to_rl(ic)))
        return err;

      if ((err = bus_generic_probe(rootdev)))
        return err;
    }
  }
  return 0;
}

int FDT_add_child(device_t *rootdev, const char *path) {
  phandle_t node;
  if ((node = FDT_finddevice(path)) == FDT_NODEV)
    return ENXIO;

  int err = 0;

  device_t *dev = device_add_child(rootdev, sb_unit++);
  dev->node = node;

  if (FDT_getencprop(node, "phandle", &dev->handle,
                     sizeof(pcell_t)) != sizeof(pcell_t)) {
    if (FDT_hasprop(node, "interrupt-controller"))
      panic("Interrupt controller without a phandle property!");
    return NULL;
  }

  dev->pic = sb_find_pic(rootdev, node);
  if (!dev->pic) {
    err = ENXIO;
    goto end;
  }

  if ((err = sb_region_to_rl(dev)))
    goto end;
  if ((err = sb_intr_to_rl(dev)))
    goto end;

  if (FDT_hasprop(node, "interrupt-controller")) {
    err = bus_generic_probe(bus);
  }

end:
  if (err)
    device_remove_child(rootdev, dev);
  return err;
}

int simplebus_enumerate(device_t *rootdev, rman_t *rm) {
  int err;

  sb_node = FDT_finddevice("/soc");
  if (sb_node == FDT_NODEV)
    return ENXIO;

  if ((err =
         FDT_addrsize_cells(sb_node, &sb_soc_addr_cells, &sb_soc_size_cells)))
    return err;

  if ((err = FDT_addrsize_cells(0, &sb_cpu_addr_cells, &sb_cpu_size_cells)))
    return err;

  rman_addr_t start, end;

  if ((err = sb_discover_io_map(&start, &end)))
    return err;

  klog("Simplebus memory region: %lx - %lx", start, end);

  rman_init(rm, "I/O region");
  rman_manage_region(rm, start, end - start);

  if ((err = sb_discover_ics(rootdev)))
    return err;

  assert(TAILQ_EMPTY(&sb_pending_ics));

  for (phandle_t node = FDT_child(sb_node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    device_t *dev = sb_new_child(node, sb_unit++);

    TAILQ_INSERT_TAIL(&rootdev->children, dev, link);

    if (FDT_hasprop(node, "interrupts") ||
        FDT_hasprop(node, "interrupts-extended")) {
      dev->pic = sb_find_pic(rootdev, dev->node);
      if (!dev->pic) {
        err = ENXIO;
        break;
      }
    }

    if ((err = sb_region_to_rl(dev)))
      break;
    if ((err = sb_intr_to_rl(dev)))
      break;
  }

  if (!err)
    return 0;

  while (!TAILQ_EMPTY(&sb_pending_ics)) {
    device_t *ic, *next;
    TAILQ_FOREACH_SAFE (ic, &rootdev->children, link, next)
      sb_remove_pending_ic(ic);
  }

  while (!TAILQ_EMPTY(&rootdev->children)) {
    device_t *dev, *next;
    TAILQ_FOREACH_SAFE (dev, &rootdev->children, link, next) {
      if (dev->bus == DEV_BUS_SIMPLEBUS)
        device_remove_child(rootdev, dev);
    }
  }
  return err;
}
