#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/klog.h>
#include <dev/fdt_dev.h>

typedef struct simplebus_ic simplebus_ic_t;
typedef TAILQ_HEAD(, simplebus_ic) simplebus_ic_list_t;

struct simplebus_ic {
  TAILQ_ENTRY(simplebus_ic) link;
  phandle_t node;
  phandle_t iparent;
  pcell_t phandle;
  bool iparent_ready;
};

typedef struct {
  fdt_mem_reg_t mrs[FDT_MAX_REG_TUPLES];
  rman_t rm;
  simplebus_ic_list_t pending_ics;
  simplebus_ic_list_t ready_ics;
  int unit;
} simplebus_t;

/*
 * FDT processing functions.
 */

static int sb_add_region(device_t *bus, phandle_t node) {
  simplebus_t *sb = bus->state;
  int err = 0;

  device_t dev = {
    .node = node,
    .parent = bus,
  };

  size_t cnt;
  if ((err = FDT_dev_get_region(&dev, sb->mrs, &cnt)))
    return err;

  for (size_t i = 0; i < cnt; i++) {
    fdt_mem_reg_t *mr = &sb->mrs[i];
    rman_manage_region(&sb->rm, mr->addr, mr->size);
  }
  return 0;
}

static phandle_t sb_find_iparent(device_t *bus, phandle_t node) {
  pcell_t iparent_phandle = FDT_get_iparent_phandle(node);

  device_t *rootdev = bus->parent;
  if (iparent_phandle == rootdev->node)
    return rootdev->node;

  for (phandle_t node = FDT_child(bus->node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    pcell_t phandle;
    if (FDT_getencprop(node, "phandle", &ic->phandle, sizeof(pcell_t)) !=
        sizeof(pcell_t))
      return FDT_NODEV;

    if (iparent_phandle = phandle)
      return phandle;
  }
  return FDT_NODEV;
}

static int sb_discover_ics(device_t *bus) {
  int nics = 0;

  for (phandle_t node = FDT_child(bus->node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    if (!FDT_hasprop(node, "interrupt-controller"))
      continue;

    simplebus_ic_t *ic =
      kmalloc(M_DEV, sizeof(simplebus_ic_t), M_WAITOK | M_ZERO);
    TAILQ_INSERT_TAIL(&sb->pending_ics, dev, link);

    ic->node = node;

    if (FDT_getencprop(node, "phandle", &ic->phandle, sizeof(pcell_t)) !=
        sizeof(pcell_t))
      return ENXIO;
  }

  device_t *rootdev = bus->parent;

  simplebus_ic_t *ic;
  TAILQ_FOREACH (ic, &sb->pending_ics, link) {
    ic->iparent = sb_find_iparent(node);
    if (ic->iparent == FDT_NODEV)
      return ENXIO;

    if (ic->iparent == rootdev->node)
      ic->iparent_ready = true;
  }

  while (!TAILQ_EMPTY(&sb->pending_ics)) {
    simplebus_ic_t *next;
    TAILQ_FOREACH_SAFE (ic, &sb->pending_ics, link, next) {
      if (!ic->iparent_ready)
        continue;

      TAILQ_REMOVE(&sb->pending_ics, ic, link);
      TAILQ_INSERT_TAIL(&sb->ready_ics, ic, link);

      simplebus_ic_t *child;
      TAILQ_FOREACH (child, &sb->pending_ics, link) {
        if (child->iparent == ic->node)
          child->iparent_ready = true;
      }
    }
  }
  return 0;
}

static int sb_add_device(device_t *bus, phandle_t node, int unit) {
  simplebus_t *sb = bus->state;
  int err = 0;

  if ((err = sb_add_region(bus, node)))
    return err;

  return FDT_dev_add_device_by_node(bus, node, sb->unit++);
}

static int sb_enumerate(device_t *bus) {
  simplebus_t *sb = bus->state;
  int err = 0;

  if ((err = sb_discover_ics(bus)))
    goto bad;

  assert(TAILQ_EMPTY(&sb->pending_ics));

  simplebus_ic_t *ic;
  TAILQ_FOREACH (ic, &sb->ready_ics, link) {
    if ((err = sb_add_device(bus, ic->node, sb->unit++)))
      goto bad;
  }

  for (phandle_t node = FDT_child(bus->node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    if (FDT_hasprop(node, "interrupt-controller"))
      continue;

    if ((err = sb_add_device(bus, node, sb->unit++)))
      goto bad;
  }

  if (!err)
    return 0;

bad:
  while (!TAILQ_EMPTY(&sb->pending_ics)) {
    device_t *ic, *next;
    TAILQ_FOREACH_SAFE (ic, &rootdev->children, link, next)
      kfree(M_DEV, ic);
  }

  while (!TAILQ_EMPTY(&bus->children)) {
    device_t *dev, *next;
    TAILQ_FOREACH_SAFE (dev, &bus->children, link, next)
      device_remove_child(rootdev, dev);
  }
  return err;
}

/*
 * Simple-bus.
 */

static int sb_activate_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_MEMORY);
  return bus_space_map(r->r_bus_tag, resource_start(r),
                       roundup(resource_size(r), PAGESIZE), &r->r_bus_handle);
}

static void sb_deactivate_resource(device_t *dev, resource_t *r) {
  /* TODO: unmap mapped resources. */
}

static resource_t *sb_alloc_resource(device_t *dev, res_type_t type, int rid,
                                     rman_addr_t start, rman_addr_t end,
                                     size_t size, rman_flags_t flags) {
  simplebus_t *sb = dev->parent->state;

  assert(type == RT_MEMORY);

  resource_t *r = rman_reserve_resource(&sb->rm, type, rid, start, end, size,
                                        sizeof(uint8_t), flags);
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

static int sb_probe(device_t *bus) {
  return FDT_is_compatible(pic->node, "simple-bus");
}

static int sb_attach(device_t *bus) {
  simplebus_t *sb = bus->state;
  int err = 0;

  TAILQ_INIT(&sb->pending_ics);
  TAILQ_INIT(&sb->ready_ics);

  rman_init(rm, "Simple-bus");

  if ((err = sb_enumerate(bus)))
    return err;

  return bus_generic_probe(bus);
}

static bus_methods_t sb_bus_if = {
  .alloc_resource = sb_alloc_resource,
  .release_resource = sb_release_resource,
  .activate_resource = sb_activate_resource,
  .deactivate_resource = sb_deactivate_resource,
};

static driver_t sb_driver = {
  .desc = "Simple-bus driver",
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
