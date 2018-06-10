#define KL_LOG KL_DEV
#include <klog.h>
#include <mips/malta.h>
#include <mips/intr.h>
#include <bus.h>
#include <exception.h>
#include <pci.h>
#include <sysinit.h>

typedef struct rootdev { void *data; } rootdev_t;

extern pci_bus_driver_t gt_pci_bus;
device_t *gt_pci;

static rman_t rm_mem;

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
  rman_create(&rm_mem, MALTA_PHYS_ADDR_SPACE_BASE, MALTA_PHYS_ADDR_SPACE_END);

  gt_pci = device_add_child(dev);
  gt_pci->driver = &gt_pci_bus.driver;
  if (device_probe(gt_pci))
    device_attach(gt_pci);
  return 0;
}

static uint8_t mips_read_1(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_start + offset;
  return *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr);
}

static void mips_write_1(resource_t *handle, unsigned offset, uint8_t value) {
  intptr_t addr = handle->r_start + offset;
  *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr) = value;
}

static uint16_t mips_read_2(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_start + offset;
  return *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(addr);
}

static void mips_write_2(resource_t *handle, unsigned offset, uint16_t value) {
  intptr_t addr = handle->r_start + offset;
  *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(addr) = value;
}

static uint32_t mips_read_4(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_start + offset;
  return *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(addr);
}

static void mips_write_4(resource_t *handle, unsigned offset, uint32_t value) {
  intptr_t addr = handle->r_start + offset;
  *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(addr) = value;
}

static void mips_read_region_1(resource_t *handle, unsigned offset,
                               uint8_t *dst, size_t count) {
  uint8_t *src = (uint8_t *)MIPS_PHYS_TO_KSEG1(handle->r_start + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

static void mips_write_region_1(resource_t *handle, unsigned offset,
                                const uint8_t *src, size_t count) {
  uint8_t *dst = (uint8_t *)MIPS_PHYS_TO_KSEG1(handle->r_start + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

static struct bus_space generic_space = {.read_1 = mips_read_1,
                                         .write_1 = mips_write_1,
                                         .read_2 = mips_read_2,
                                         .write_2 = mips_write_2,
                                         .read_4 = mips_read_4,
                                         .write_4 = mips_write_4,
                                         .read_region_1 = mips_read_region_1,
                                         .write_region_1 = mips_write_region_1};

static resource_t *rootdev_resource_alloc(device_t *bus, device_t *child,
                                          int type, int rid, rman_addr_t start,
                                          rman_addr_t end, rman_addr_t size,
                                          unsigned flags) {

  resource_t *r =
    rman_allocate_resource(&rm_mem, start, end, size, size, RF_NONE);
  r->r_owner = child;
  r->r_bus_space = &generic_space;
  r->r_type = type;
  LIST_INSERT_HEAD(&child->resources, r, r_device);
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
