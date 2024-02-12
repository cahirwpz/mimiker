#define KL_LOG KL_DEV
#include <sys/errno.h>
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/fdt.h>
#include <sys/libkern.h>
#include <aarch64/armreg.h>
#include <dev/bcm2835reg.h>
#include <dev/fdt_dev.h>

/*
 * located at BCM2836_ARM_LOCAL_BASE
 * 32 local interrupts -- one per CPU but now we only support 1 CPU
 */

typedef struct rootdev {
  dev_mmio_t mem_local; /* ARM local */
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

static void rootdev_setup_intr(device_t *pic, device_t *dev, dev_intr_t *intr,
                               ih_filter_t *filter, ih_service_t *service,
                               void *arg, const char *name) {
  rootdev_t *rd = pic->state;
  unsigned irq = intr->irq;
  assert(irq < BCM2836_INT_NLOCAL);

  if (rd->intr_event[irq] == NULL)
    rd->intr_event[irq] = intr_event_create(
      rd, irq, rootdev_disable_irq, rootdev_enable_irq, rootdev_intr_name(irq));

  intr->handler =
    intr_event_add_handler(rd->intr_event[irq], filter, service, arg, name);
}

static void rootdev_teardown_intr(device_t *pic, device_t *dev,
                                  dev_intr_t *intr) {
  intr_event_remove_handler(intr->handler);
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
                            size_t icells) {
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

DEVCLASS_DECLARE(root);

static int rootdev_attach(device_t *bus) {
  rootdev_t *rd = bus->state;

  bus->devclass = &DEVCLASS(root);

  phandle_t node = FDT_finddevice("/soc/local_intc");
  if (node == FDT_NODEV)
    return ENXIO;
  bus->node = node;

  rd->mem_local = (dev_mmio_t){
    .bus_tag = generic_bus_space,
  };
  int err = bus_space_map(generic_bus_space, BCM2836_ARM_LOCAL_BASE,
                          BCM2836_ARM_LOCAL_SIZE, &rd->mem_local.bus_handle);
  if (err)
    return ENXIO;

  /*
   * Device enumeration.
   * TODO: this should be performed by a simplebus enumeration.
   */

  if ((err = FDT_dev_add_child(bus, "/timer", DEV_BUS_FDT)))
    return err;

  if ((err = FDT_dev_add_child(bus, "/soc/gpio", DEV_BUS_FDT)))
    return err;

  if ((err = FDT_dev_add_child(bus, "/soc/serial", DEV_BUS_FDT)))
    return err;

  if ((err = FDT_dev_add_child(bus, "/soc/emmc", DEV_BUS_FDT)))
    return err;

  if ((err = FDT_dev_add_child(bus, "/soc/intc", DEV_BUS_FDT)))
    return err;

  intr_pic_register(bus, node);
  intr_root_claim(rootdev_intr_handler, bus);

  return 0;
}

static int rootdev_map_mmio(device_t *dev, dev_mmio_t *mmio) {
  rootdev_t *rd = dev->parent->state;
  bus_addr_t addr = mmio->start;
  bus_size_t size = dev_mmio_size(mmio);

  bus_addr_t arm_local = BCM2836_ARM_LOCAL_BASE;

  if (addr >= arm_local && addr + size <= arm_local + BCM2836_ARM_LOCAL_SIZE) {
    bus_size_t off = addr - arm_local;
    mmio->bus_handle = rd->mem_local.bus_handle + off;
    return 0;
  }

  mmio->bus_tag = generic_bus_space;

  return bus_space_map(mmio->bus_tag, addr, size, &mmio->bus_handle);
}

static void rootdev_unmap_mmio(device_t *dev, dev_mmio_t *mmio) {
  /* TODO: unmap mapped resources. */
}

static bus_methods_t rootdev_bus_if = {
  .map_mmio = rootdev_map_mmio,
  .unmap_mmio = rootdev_unmap_mmio,
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
