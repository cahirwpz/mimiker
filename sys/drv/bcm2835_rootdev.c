#define KL_LOG KL_DEV
#include <sys/errno.h>
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/fdt.h>
#include <sys/libkern.h>
#include <aarch64/armreg.h>
#include <dev/bcm2835reg.h>
#include <dev/simplebus.h>

/*
 * located at BCM2836_ARM_LOCAL_BASE
 * 32 local interrupts -- one per CPU but now we only support 1 CPU
 */

typedef struct rootdev {
  resource_t mem_local; /* ARM local */
  intr_event_t *intr_event[BCM2836_NIRQ];
} rootdev_t;

#define in4(addr) bus_read_4(&rd->mem_local, (addr))
#define out4(addr, val) bus_write_4(&rd->mem_local, (addr), (val))

static void rootdev_enable_irq(intr_event_t *ie) {
  rootdev_t *rd = ie->ie_source;
  unsigned irq = ie->ie_irq;
  assert(irq < BCM2836_INT_NLOCAL);

  /* TODO: fix needed for SMP. */
  uint32_t irqctrl = in4(BCM2836_LOCAL_TIMER_IRQ_CONTROLN(0));
  out4(BCM2836_LOCAL_TIMER_IRQ_CONTROLN(0), irqctrl | (1 << irq));
}

static void rootdev_disable_irq(intr_event_t *ie) {
  rootdev_t *rd = ie->ie_source;
  unsigned irq = ie->ie_irq;
  assert(irq < BCM2836_INT_NLOCAL);

  /* TODO: fix needed for SMP. */
  uint32_t irqctrl = in4(BCM2836_LOCAL_TIMER_IRQ_CONTROLN(0));
  out4(BCM2836_LOCAL_TIMER_IRQ_CONTROLN(0), irqctrl & ~(1 << irq));
}

static const char *rootdev_intr_name(int irq) {
  return kasprintf("ARM local interrupt source %d", irq);
}

static void rootdev_setup_intr(device_t *pic, device_t *dev, resource_t *r,
                               ih_filter_t *filter, ih_service_t *service,
                               void *arg, const char *name) {
  rootdev_t *rd = pic->state;
  int irq = r->r_irq;
  assert(irq < BCM2836_INT_NLOCAL);

  if (rd->intr_event[irq] == NULL)
    rd->intr_event[irq] = intr_event_create(
      rd, irq, rootdev_disable_irq, rootdev_enable_irq, rootdev_intr_name(irq));

  r->r_handler =
    intr_event_add_handler(rd->intr_event[irq], filter, service, arg, name);
}

static void rootdev_teardown_intr(device_t *pic, device_t *dev,
                                  resource_t *irq) {
  intr_event_remove_handler(irq->r_handler);
}

static void rootdev_intr_handler(ctx_t *ctx, device_t *dev) {
  rootdev_t *rd = dev->state;
  intr_event_t **events = rd->intr_event;
  /* TODO: fix needed for SMP. */
  uint32_t pending = in4(BCM2836_LOCAL_INTC_IRQPENDINGN(0));

  while (pending) {
    unsigned irq = ffs(pending) - 1;
    if (events[irq])
      intr_event_run_handlers(events[irq]);
    pending &= ~(1 << irq);
  }
}

static int rootdev_map_intr(device_t *pic, device_t *dev, phandle_t *intr,
                            int icells) {
  if (icells != 2)
    return -1;

  unsigned irq = *intr;
  if (irq >= BCM2836_INT_NLOCAL)
    return -1;
  return irq;
}

static int rootdev_probe(device_t *bus) {
  return 1;
}

DEVCLASS_DECLARE(emmc);

static int rootdev_attach(device_t *bus) {
  rootdev_t *rd = bus->state;

  bus->node = FDT_finddevice("/soc/local_intc");
  if (bus->node == FDT_NODEV)
    return ENXIO;

  rd->mem_local = (resource_t){
    .r_type = RT_MEMORY,
    .r_bus_tag = generic_bus_space,
  };
  int err = bus_space_map(generic_bus_space, BCM2836_ARM_LOCAL_BASE,
                          BCM2836_ARM_LOCAL_SIZE, &rd->mem_local.r_bus_handle);
  if (err)
    return ENXIO;

  intr_root_claim(rootdev_intr_handler, bus);

  /*
   * Device enumeration.
   * TODO: this should be performed by a simplebus enumeration.
   */

  int unit = 0;
  device_t *bcm2835_pic, *emmc;

  if ((err = simplebus_add_child(bus, "/soc/intc", unit++, bus, &bcm2835_pic)))
    return err;

  if ((err = simplebus_add_child(bus, "/timer", unit++, bus, NULL)))
    return err;

  if ((err = simplebus_add_child(bus, "/soc/gpio", unit++, bcm2835_pic, NULL)))
    return err;

  if ((err =
         simplebus_add_child(bus, "/soc/serial", unit++, bcm2835_pic, NULL)))
    return err;

  if ((err = simplebus_add_child(bus, "/soc/emmc", unit++, bcm2835_pic, &emmc)))
    return err;
  emmc->devclass = &DEVCLASS(emmc);

  return bus_generic_probe(bus);
}

static int rootdev_map_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_MEMORY);
  rootdev_t *rd = dev->parent->state;
  bus_addr_t addr = r->r_start;
  bus_size_t size = resource_size(r);

  bus_addr_t arm_local = BCM2836_ARM_LOCAL_BASE;

  if (addr >= arm_local && addr + size <= arm_local + BCM2836_ARM_LOCAL_SIZE) {
    bus_size_t off = addr - arm_local;
    r->r_bus_handle = rd->mem_local.r_bus_handle + off;
    return 0;
  }

  r->r_bus_tag = generic_bus_space;

  return bus_space_map(r->r_bus_tag, addr, size, &r->r_bus_handle);
}

static void rootdev_unmap_resource(device_t *dev, resource_t *r) {
  assert(r->r_type == RT_MEMORY);
  /* TODO: unmap mapped resources. */
}

static bus_methods_t rootdev_bus_if = {
  .map_resource = rootdev_map_resource,
  .unmap_resource = rootdev_unmap_resource,
};

static pic_methods_t rootdev_pic_if = {
  .setup_intr = rootdev_setup_intr,
  .teardown_intr = rootdev_teardown_intr,
  .map_intr = rootdev_map_intr,
};

driver_t rootdev_driver = {
  .desc = "RPI3 platform root bus driver",
  .size = sizeof(rootdev_t),
  .pass = FIRST_PASS,
  .probe = rootdev_probe,
  .attach = rootdev_attach,
  .interfaces =
    {
      [DIF_BUS] = &rootdev_bus_if,
      [DIF_PIC] = &rootdev_pic_if,
    },
};

DEVCLASS_CREATE(root);
