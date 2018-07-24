#define KL_LOG KL_DEV
#include <klog.h>
#include <mips/malta.h>
#include <mips/intr.h>
#include <bus.h>
#include <exception.h>
#include <pci.h>
#include <sysinit.h>

typedef struct rootdev { void *data; } rootdev_t;

/* TODO: remove following lines when devclasses are implemented */
extern pci_bus_driver_t gt_pci_bus;
device_t *gt_pci;

static rman_t rm_mem; /* stores all resources of root bus children */

static inline rootdev_t *rootdev_of(device_t *dev) {
  return dev->instance;
}

static void rootdev_intr_setup(device_t *dev, unsigned num,
                               intr_handler_t *handler) {
  mips_intr_setup(handler, num);
}

static void rootdev_intr_teardown(device_t *dev, intr_handler_t *handler) {
  mips_intr_teardown(handler);
}

static int rootdev_attach(device_t *dev) {
  /* Manages space occupied by I/O devices: PCI, FPGA, system controler, ... */
  rman_create(&rm_mem, 0x10000000, 0x1fffffff, RT_MEMORY);

  gt_pci = device_add_child(dev);
  gt_pci->driver = &gt_pci_bus.driver;
  if (device_probe(gt_pci))
    device_attach(gt_pci);
  return 0;
}

static uint8_t mips_read_1(bus_addr_t addr, off_t offset) {
  return *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr + offset);
}

static uint16_t mips_read_2(bus_addr_t addr, off_t offset) {
  return *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(addr + offset);
}

static uint32_t mips_read_4(bus_addr_t addr, off_t offset) {
  return *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(addr + offset);
}

static void mips_write_1(bus_addr_t addr, off_t offset, uint8_t value) {
  *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr + offset) = value;
}

static void mips_write_2(bus_addr_t addr, off_t offset, uint16_t value) {
  *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(addr + offset) = value;
}

static void mips_write_4(bus_addr_t addr, off_t offset, uint32_t value) {
  *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(addr + offset) = value;
}

static void mips_read_region_1(bus_addr_t addr, off_t offset, uint8_t *dst,
                               size_t count) {
  uint8_t *src = (uint8_t *)MIPS_PHYS_TO_KSEG1(addr + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

static void mips_write_region_1(bus_addr_t addr, off_t offset,
                                const uint8_t *src, size_t count) {
  uint8_t *dst = (uint8_t *)MIPS_PHYS_TO_KSEG1(addr + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

static bus_space_t generic_space = {.bs_read_1 = mips_read_1,
                                    .bs_read_2 = mips_read_2,
                                    .bs_read_4 = mips_read_4,
                                    .bs_write_1 = mips_write_1,
                                    .bs_write_2 = mips_write_2,
                                    .bs_write_4 = mips_write_4,
                                    .bs_read_region_1 = mips_read_region_1,
                                    .bs_write_region_1 = mips_write_region_1};

static resource_t *rootdev_resource_alloc(device_t *bus, device_t *child,
                                          res_type_t type, int rid,
                                          rman_addr_t start, rman_addr_t end,
                                          size_t size, unsigned flags) {

  resource_t *r = rman_alloc_resource(&rm_mem, start, end, size, 1, 0, child);

  if (r)
    device_add_resource(child, r, rid, &generic_space);

  return r;
}

static bus_driver_t rootdev_driver = {
  .driver =
    {
      .size = sizeof(rootdev_t),
      .desc = "MIPS platform root bus driver",
      .attach = rootdev_attach,
    },
  .bus = {.intr_setup = rootdev_intr_setup,
          .intr_teardown = rootdev_intr_teardown,
          .resource_alloc = rootdev_resource_alloc}};

static device_t rootdev = (device_t){
  .children = TAILQ_HEAD_INITIALIZER(rootdev.children),
  .driver = (driver_t *)&rootdev_driver,
  .instance = &(rootdev_t){},
  .state = NULL,
};

static void rootdev_init(void) {
  device_attach(&rootdev);
}

bus_space_t *mips_bus_space_generic = &generic_space;

SYSINIT_ADD(rootdev, rootdev_init, DEPS("mount_fs"));
