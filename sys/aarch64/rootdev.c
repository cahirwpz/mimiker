#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <aarch64/bcm2835reg.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/interrupt.h>

/*
 * located at BCM2836_ARM_LOCAL_BASE
 * 32 local interrupts -- one per CPU but now we only support 1 CPU
 *
 * located at BCM2835_ARMICU_BASE
 * accessed by BCM2835_PERIPHERALS_BASE NOT by BCM2835_PERIPHERALS_BASE_BUS
 * 32 GPU0 interrupts -- 0x204 offset from address; 32 in table
 * 32 GPU1 interrupts -- 0x208 offset from address; 64 in table
 * 32 base interrupts -- 0x200 offset from address; 96 in table
 */

#define NIRQ (BCM2835_NIRQ + BCM2836_NIRQ)

typedef struct rootdev {
  rman_t local_rm;
  rman_t shared_rm;
  intr_event_t intr_event[NIRQ];
  vaddr_t arm_base;
} rootdev_t;

/* BCM2836_ARM_LOCAL_BASE mapped into VA (4KiB). */
static vaddr_t rootdev_local_handle;

/* Use pre mapped memory. */
static int rootdev_bs_map(bus_addr_t addr, bus_size_t size,
                          bus_space_handle_t *handle_p) {
  if (addr >= BCM2836_ARM_LOCAL_BASE &&
      addr < BCM2836_ARM_LOCAL_BASE + BCM2836_ARM_LOCAL_SIZE) {
    *handle_p = rootdev_local_handle;
    return 0;
  }
  return generic_bs_map(addr, size, handle_p);
}

/* clang-format off */
static bus_space_t *rootdev_bus_space = &(bus_space_t){
  .bs_map = rootdev_bs_map,
  .bs_read_1 = generic_bs_read_1,
  .bs_read_2 = generic_bs_read_2,
  .bs_read_4 = generic_bs_read_4,
  .bs_write_1 = generic_bs_write_1,
  .bs_write_2 = generic_bs_write_2,
  .bs_write_4 = generic_bs_write_4,
  .bs_read_region_1 = generic_bs_read_region_1,
  .bs_read_region_2 = generic_bs_read_region_2,
  .bs_read_region_4 = generic_bs_read_region_4,
  .bs_write_region_1 = generic_bs_write_region_1,
  .bs_write_region_2 = generic_bs_write_region_2,
  .bs_write_region_4 = generic_bs_write_region_4,
};
/* clang-format on */

static void rootdev_intr_setup(device_t *dev, unsigned num,
                               intr_handler_t *handler) {
  rootdev_t *rd = dev->parent->state;

  assert(num < NIRQ);
  intr_event_t *event = &rd->intr_event[num];
  intr_event_add_handler(event, handler);
}

static void rootdev_intr_teardown(device_t *dev, intr_handler_t *handler) {
  intr_event_remove_handler(handler);
}

/* Read 32 bit pending register located at va and run handlers. */
static void bcm2835_intr_handle(vaddr_t irqpendr, intr_event_t *events) {
  uint32_t pending = *(uint32_t *)irqpendr;

  while (pending) {
    int irq = ffs(pending) - 1;
    intr_event_run_handlers(&events[irq]);
    pending &= ~(1 << irq);
  }
}

static void rootdev_intr_handler(device_t *dev, void *arg) {
  assert(dev != NULL);
  rootdev_t *rd = dev->state;

  /* Handle local interrupts. */
  bcm2835_intr_handle(rootdev_local_handle + BCM2836_LOCAL_INTC_IRQPENDINGN(0),
                      &rd->intr_event[BCM2836_INT_BASECPUN(0)]);

  /* Handle GPU0 interrupts. */
  bcm2835_intr_handle(rd->arm_base +
                        (BCM2835_ARMICU_OFFSET + BCM2835_INTC_IRQ1PENDING),
                      &rd->intr_event[BCM2835_INT_GPU0BASE]);

  /* Handle GPU1 interrupts. */
  bcm2835_intr_handle(rd->arm_base +
                        (BCM2835_ARMICU_OFFSET + BCM2835_INTC_IRQ2PENDING),
                      &rd->intr_event[BCM2835_INT_GPU1BASE]);

  /* Handle base interrupts. */
  bcm2835_intr_handle(rd->arm_base +
                        (BCM2835_ARMICU_OFFSET + BCM2835_INTC_IRQBPENDING),
                      &rd->intr_event[BCM2835_INT_BASICBASE]);
}

static int rootdev_attach(device_t *bus) {
  rootdev_t *rd = bus->state;

  /* TODO(cahir) Resource manager should be able to manage independant ranges
   * instead of single one. Consult rman_manage_region in rman(9). */
  rman_init(&rd->shared_rm, "BCM2835 peripherals", BCM2835_PERIPHERALS_BASE,
            BCM2835_PERIPHERALS_BASE + BCM2835_PERIPHERALS_SIZE - 1, RT_MEMORY);
  rman_init(&rd->local_rm, "ARM local", BCM2836_ARM_LOCAL_BASE,
            BCM2836_ARM_LOCAL_BASE + BCM2836_ARM_LOCAL_SIZE - 1, RT_MEMORY);

  /* Map BCM2836 shared processor only once. */
  rootdev_local_handle =
    kmem_map(BCM2836_ARM_LOCAL_BASE, BCM2836_ARM_LOCAL_SIZE, PMAP_NOCACHE);

  rd->arm_base = kmem_map(BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_ARM_BASE),
                          PAGESIZE, PMAP_NOCACHE);

  for (int i = 0; i < NIRQ; i++) {
    intr_event_init(&rd->intr_event[i], i, NULL, NULL, NULL, NULL);
    intr_event_register(&rd->intr_event[i]);
  }

  intr_root_claim(rootdev_intr_handler, bus, NULL);

  return bus_generic_probe(bus);
}

static resource_t *rootdev_alloc_resource(device_t *bus, device_t *child,
                                          res_type_t type, int rid,
                                          rman_addr_t start, rman_addr_t end,
                                          size_t size, res_flags_t flags) {
  rootdev_t *rd = bus->state;
  resource_t *r;

  r = rman_alloc_resource(&rd->local_rm, start, end, size, 1, RF_NONE, child);
  if (r == NULL)
    r =
      rman_alloc_resource(&rd->shared_rm, start, end, size, 1, RF_NONE, child);

  if (r) {
    r->r_bus_tag = rootdev_bus_space;
    if (flags & RF_ACTIVE)
      bus_space_map(r->r_bus_tag, r->r_start, r->r_end - r->r_start + 1,
                    &r->r_bus_handle);
  }

  return r;
}

static bus_driver_t rootdev_driver = {
  .driver =
    {
      .size = sizeof(rootdev_t),
      .desc = "RPI3 platform root bus driver",
      .attach = rootdev_attach,
    },
  .bus =
    {
      .intr_setup = rootdev_intr_setup,
      .intr_teardown = rootdev_intr_teardown,
      .alloc_resource = rootdev_alloc_resource,
    },
};

DEVCLASS_CREATE(root);

static device_t rootdev = (device_t){
  .children = TAILQ_HEAD_INITIALIZER(rootdev.children),
  .driver = (driver_t *)&rootdev_driver,
  .state = &(rootdev_t){},
  .devclass = &DEVCLASS(root),
};

DEVCLASS_ENTRY(root, rootdev);

void init_devices(void) {
  device_attach(&rootdev);
}
