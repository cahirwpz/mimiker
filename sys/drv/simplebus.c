#define KL_LOG KL_DEV
#include <sys/devclass.h>
#include <sys/errno.h>
#include <sys/bus.h>
#include <sys/fdt.h>
#include <sys/klog.h>
#include <sys/rman.h>

typedef struct simplebus {
  device_list_t pending_ics;
  rman_addr_t io_start;
  rman_addr_t io_end;
  rman_t mem_rm;
  size_t nics;
} simplebus_t;

static device_t *sb_new_child(phandle_t node, int unit) {
  device_t *dev = device_alloc(unit);
  dev->bus = DEV_BUS_SIMPLEBUS;
  dev->node = node;
  return dev;
}

static int sb_read_io_map(device_t *bus) {
  simplebus_t *sb = bus->state;
  fdt_mem_reg_t mrs[FDT_MAX_REG_TUPLES];
  size_t cnt;
  int err = 0;

  sb->io_start = RMAN_ADDR_MAX;
  sb->io_end = 0;

  for (phandle_t node = FDT_child(bus->node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    if ((err = FDT_get_reg(node, mrs, &cnt)))
      return err;

    for (size_t i = 0; i < cnt; i++) {
      u_long start = mrs[i].addr;
      u_long end = mrs[i].addr + mrs[i].size;
      assert(start < end);
      if (start < sb->io_start)
        sb->io_start = start;
      if (end > sb->io_end)
        sb->io_end = end;
    }
  }
  assert((sb->io_start != RMAN_ADDR_MAX) && sb->io_end);

  return 0;
}

static phandle_t sb_find_iparent(phandle_t node) {
  phandle_t iparent;

  if (FDT_searchencprop(node, "interrupt-parent", &iparent,
                        sizeof(phandle_t)) == sizeof(phandle_t))
    return iparent;

  for (; node != FDT_NODEV; node = FDT_peer(node))
    if (FDT_hasprop(node, "interrupt-controller"))
      break;
  return node;
}

static device_t *sb_find_pic(device_t *bus, device_t *dev) {
  simplebus_t *sb = bus->state;
  phandle_t iparent = sb_find_iparent(dev->node);
  if (iparent == FDT_NODEV)
    return NULL;

  device_t *cpu_ic = bus->parent;
  if (dev->node == cpu_ic->node)
    return cpu_ic;

  device_t *child;
  device_list_t *list = &sb->pending_ics;
  if (TAILQ_EMPTY(list))
    list = &bus->children;

  TAILQ_FOREACH (child, list, link) {
    if (child->node == iparent)
      return child;
  }
  return NULL;
}

static int sb_reg_to_rl(device_t *dev) {
  fdt_mem_reg_t mrs[FDT_MAX_REG_TUPLES];
  size_t cnt;

  int err = FDT_get_reg(dev->node, mrs, &cnt);
  if (err)
    return err;

  for (size_t i = 0; i < cnt; i++)
    device_add_memory(dev, i, mrs[i].addr, mrs[i].size);

  return 0;
}

static int sb_intr_to_rl(device_t *dev) {
  assert(dev->pic);
  assert(dev->pic->driver);

  phandle_t node = dev->node;
  phandle_t iparent;
  pcell_t *intr;
  int err = 0;
  int icells;
  bool extended;

  int nintrs = FDT_getencprop_alloc_multi(node, "interrupts", sizeof(pcell_t),
                                          (void **)&intr);
  if (nintrs > 0) {
    iparent = sb_find_iparent(node);
    if (iparent == FDT_NODEV) {
      klog("Failed to find interrupt-parent for device node %d", node);
      err = ENXIO;
      goto end;
    }
    if ((err = FDT_intr_cells(node, &icells)))
      goto end;
    if (icells > nintrs) {
      err = ERANGE;
      goto end;
    }
    extended = false;
  } else {
    nintrs = FDT_getencprop_alloc_multi(node, "interrupts-extended",
                                        sizeof(pcell_t), (void **)&intr);
    if (nintrs < 0) {
      klog("Device node %d has invalid interrupts and interrupts-extended "
           "properties",
           node);
      return ENXIO;
    }
    if (!nintrs)
      return 0;
    extended = true;
  }

  for (int i = 0; i < nintrs; i += icells) {
    if (extended) {
      iparent = intr[i++];
      if ((err = FDT_intr_cells(node, &icells)))
        goto end;
      if ((i + icells) > nintrs) {
        err = ERANGE;
        goto end;
      }
      int irqnum = pic_map_intr(dev, &intr[i], icells);
      if (irqnum >= 0)
        device_add_irq(dev, i, irqnum);
    }
  }

end:
  FDT_free(intr);
  return err;
}

static int sb_discover_ics(device_t *bus) {
  simplebus_t *sb = bus->state;
  int err = 0;

  for (phandle_t node = FDT_child(bus->node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    if (!FDT_hasprop(node, "interrupt-controller"))
      continue;
    device_t *dev = sb_new_child(node, sb->nics++);
    TAILQ_INSERT_TAIL(&sb->pending_ics, dev, link);
  }

  device_t *ic;
  TAILQ_FOREACH (ic, &sb->pending_ics, link) {
    ic->pic = sb_find_pic(bus, ic);
    if (!ic->pic)
      return ENXIO;
  }

  while (!TAILQ_EMPTY(&sb->pending_ics)) {
    device_t *next;
    TAILQ_FOREACH_SAFE (ic, &sb->pending_ics, link, next) {
      if (!ic->pic->driver)
        continue;
      TAILQ_REMOVE(&sb->pending_ics, ic, link);
      TAILQ_INSERT_TAIL(&bus->children, ic, link);
      sb_reg_to_rl(ic);
      sb_intr_to_rl(ic);
      if ((err = bus_generic_probe(bus)))
        return err;
    }
  }
  return err;
}

static resource_t *sb_alloc_resource(device_t *dev, res_type_t type, int rid,
                                     rman_addr_t start, rman_addr_t end,
                                     size_t size, rman_flags_t flags) {
  simplebus_t *sb = dev->parent->state;
  rman_t *rman = &sb->mem_rm;
  size_t alignment = PAGESIZE;

  assert(type == RT_MEMORY);

  resource_t *r =
    rman_reserve_resource(rman, type, rid, start, end, size, alignment, flags);
  if (!r)
    return NULL;

  r->r_bus_tag = generic_bus_space;
  r->r_bus_handle = resource_start(r);

  if (flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, r)) {
      resource_release(r);
      return NULL;
    }
  }

  return r;
}

static void sb_release_resource(device_t *dev, resource_t *r) {
  bus_deactivate_resource(dev, r);
  resource_release(r);
}

static int sb_activate_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_MEMORY);
  return bus_space_map(r->r_bus_tag, resource_start(r),
                       roundup(resource_size(r), PAGESIZE), &r->r_bus_handle);
}

static void sb_deactivate_resource(device_t *dev, resource_t *r) {
  /* TODO: unmap mapped resources. */
}

static int sb_probe(device_t *bus) {
  return FDT_is_compatible(bus->node, "simple-bus");
}

static int sb_attach(device_t *bus) {
  simplebus_t *sb = bus->state;
  int err = 0;

  TAILQ_INIT(&sb->pending_ics);

  if ((err = sb_read_io_map(bus)))
    return err;

  rman_init(&sb->mem_rm, "I/O region");
  rman_manage_region(&sb->mem_rm, sb->io_start, sb->io_end - sb->io_start);

  if ((err = sb_discover_ics(bus)))
    return err;

  assert(TAILQ_EMPTY(&sb->pending_ics));

  int unit = sb->nics;
  for (phandle_t node = FDT_child(bus->node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    device_t *dev = sb_new_child(node, unit++);
    TAILQ_INSERT_TAIL(&bus->children, dev, link);
    sb_reg_to_rl(dev);
    if (FDT_hasprop(node, "interrupts") ||
        FDT_hasprop(node, "interrupts-extended")) {
      dev->pic = sb_find_pic(bus, dev);
      if (!dev->pic)
        return ENXIO;
      sb_intr_to_rl(dev);
    }
  }

  return bus_generic_probe(bus);
}

static bus_methods_t sb_bus_if = {
  .alloc_resource = sb_alloc_resource,
  .release_resource = sb_release_resource,
  .activate_resource = sb_activate_resource,
  .deactivate_resource = sb_deactivate_resource,
};

static driver_t sb_driver = {
  .desc = "Simple bus driver",
  .size = sizeof(simplebus_t),
  .pass = FIRST_PASS,
  .probe = sb_probe,
  .attach = sb_attach,
  .interfaces =
    {
      [DIF_BUS] = &sb_bus_if,
    },
};

DEVCLASS_CREATE(simplebus);
DEVCLASS_ENTRY(root, sb_driver);
