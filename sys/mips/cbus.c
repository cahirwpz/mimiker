#define KL_LOG KL_TEST
#include <mips/malta.h>
#include <mips/mips.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/klog.h>

typedef struct cbus_state {
  resource_t *cbus_io;
  resource_t *cbus_irq;

  rman_t cbus_io_rman;
  rman_t cbus_irq_rman;
  intr_event_t *intr_event[1];

} cbus_state_t;

DEVCLASS_DECLARE(cbus);
DEVCLASS_DECLARE(root);

static intr_filter_t cbus_intr(void *data) {
  klog("cbus intr\n");
  cbus_state_t *cbs = data;
  intr_event_run_handlers(cbs->intr_event[0]);
  return IF_FILTERED;
}

static int cbus_attach(device_t *cbus_dev) {
  cbus_state_t *cbs = cbus_dev->state;

  cbs->cbus_io = device_take_memory(cbus_dev, 0, RF_ACTIVE);

  cbs->cbus_irq = device_take_irq(cbus_dev, 0, RF_ACTIVE);
  bus_intr_setup(cbus_dev, cbs->cbus_irq, cbus_intr, NULL, cbs,
                 "CBUS main irq");

  assert(cbs->cbus_io);
  assert(cbs->cbus_irq);

  rman_init(&cbs->cbus_io_rman, "CBUS IO");
  rman_manage_region(&cbs->cbus_io_rman, 0, 0x1000);

  rman_init(&cbs->cbus_irq_rman, "CBUS interrupts");
  rman_manage_region(&cbs->cbus_irq_rman, 0, 1);

  device_t *dev;

  /* ns16550 uart device */
  dev =
    device_add_child(cbus_dev, 1); /* 1 for now to be interchangable with pci */
  dev->bus = DEV_BUS_CBUS;
  device_add_ioports(dev, 0, MALTA_CBUS_UART_OFFSET, 8);
  device_add_irq(dev, 0, 0);

  return bus_generic_probe(cbus_dev);
}

static resource_t *cbus_alloc_resource(device_t *dev, res_type_t type, int rid,
                                       rman_addr_t start, rman_addr_t end,
                                       size_t size, res_flags_t flags) {
  rman_t *rman = NULL;
  cbus_state_t *cbs = dev->parent->state;
  bus_space_handle_t bh = 0;

  if (type == RT_IRQ) {
    assert(dev->bus == DEV_BUS_CBUS);
    rman = &cbs->cbus_irq_rman;
  } else if (type == RT_IOPORTS) {
    rman = &cbs->cbus_io_rman;
    bh = cbs->cbus_io->r_bus_handle;
  } else {
    panic("Unknown CBUS resource of type %d", type);
  }

  resource_t *r = rman_reserve_resource(rman, start, end, size, size, flags);
  if (r == NULL)
    return NULL;

  r->r_rid = rid;

  if (type != RT_IRQ) {
    r->r_bus_tag = generic_bus_space;
    r->r_bus_handle = bh + r->r_start;
  }

  if (flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, type, r)) {
      rman_release_resource(r);
      return NULL;
    }
  }

  return r;
}

static int cbus_activate_resource(device_t *dev, res_type_t type,
                                  resource_t *r) {
  return 0;
}

static void cbus_release_resource(device_t *dev, res_type_t type,
                                  resource_t *r) {
  rman_release_resource(r);
}

static void cbus_mask_irq(intr_event_t *ie) {
  // TODO
  return;
}

static void cbus_unmask_irq(intr_event_t *ie) {
  // TODO
  return;
}

static void cbus_intr_setup(device_t *dev, resource_t *r, ih_filter_t *filter,
                            ih_service_t *service, void *arg,
                            const char *name) {
  cbus_state_t *cbs = dev->parent->state;
  int irq = r->r_start;

  if (cbs->intr_event[irq] == NULL)
    cbs->intr_event[irq] =
      intr_event_create(cbs, irq, cbus_mask_irq, cbus_unmask_irq, "cbus intr");

  r->r_handler =
    intr_event_add_handler(cbs->intr_event[irq], filter, service, arg, name);
}

static void cbus_intr_teardown(device_t *isab, resource_t *irq) {
  intr_event_remove_handler(irq->r_handler);
}

static void cbus_deactivate_resource(device_t *dev, res_type_t type,
                                     resource_t *r) {
  return; // TODO
}

static int cbus_probe(device_t *d) {
  return d->unit == 2;
}

typedef struct cbus_driver {
  driver_t driver;
  bus_methods_t bus;
} cbus_driver_t;

static bus_methods_t cbus_bus_if = {
  .intr_setup = cbus_intr_setup,
  .intr_teardown = cbus_intr_teardown,
  .alloc_resource = cbus_alloc_resource,
  .activate_resource = cbus_activate_resource,
  .deactivate_resource = cbus_deactivate_resource,
  .release_resource = cbus_release_resource,
};

static driver_t cbus_driver = {
  .desc = "CBUS Driver",
  .size = sizeof(cbus_state_t),
  .attach = cbus_attach,
  .probe = cbus_probe,
  .interfaces =
    {
      [DIF_BUS] = &cbus_bus_if,
    },
};

DEVCLASS_ENTRY(root, cbus_driver);
