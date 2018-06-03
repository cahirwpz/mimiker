#define KL_LOG KL_DEV
#include <klog.h>
#include <mips/malta.h>
#include <mips/intr.h>
#include <bus.h>
#include <exception.h>
#include <pci.h>
#include <sysinit.h>

typedef struct rootdev { void *data; } rootdev_t;

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

extern pci_bus_driver_t gt_pci_bus;
device_t *gt_pci;

static int rootdev_attach(device_t *dev) {
  rman_create(&rm_mem, MALTA_PHYS_ADDR_SPACE_BASE, MALTA_PHYS_ADDR_SPACE_END);

  gt_pci = device_add_child(dev);
  gt_pci->driver = &gt_pci_bus.driver;
  if (device_probe(gt_pci))
    device_attach(gt_pci);
  return 0;
}

static resource_t *rootdev_resource_alloc(device_t *rootdev, device_t *dev,
                                          unsigned flags, rm_res_t start,
                                          rm_res_t end, rm_res_t size) {

  kprintf("dupa: %s allocates resource [%lx, %lx] of size %ld for %s\n", 
    rootdev->driver->desc, start, end, size, dev->driver->desc);
  return rman_allocate_resource(&rm_mem, start, end, size);
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

SYSINIT_ADD(rootdev, rootdev_init, DEPS("mount_fs"));
