#define KL_LOG KL_DEV
#include <klog.h>
#include <mips/malta.h>
#include <mips/intr.h>
#include <bus.h>
#include <sync.h>
#include <exception.h>
#include <pci.h>
#include <sysinit.h>

typedef struct rootdev { void *data; } rootdev_t;

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

extern pci_bus_driver_t gt_pci;

static int rootdev_attach(device_t *dev) {
  device_t *pcib = device_add_child(dev);
  pcib->driver = &gt_pci.driver;
  if (device_probe(pcib))
    device_attach(pcib);
  return 0;
}

static bus_driver_t rootdev_driver = {
  .driver =
    {
      .size = sizeof(rootdev_t),
      .desc = "MIPS platform root bus driver",
      .attach = rootdev_attach,
    },
  .bus =
    {
      .intr_setup = rootdev_intr_setup, .intr_teardown = rootdev_intr_teardown,
    }};

static device_t rootdev = (device_t){
  .children = TAILQ_HEAD_INITIALIZER(rootdev.children),
  .driver = (driver_t *)&rootdev_driver,
  .instance = &(rootdev_t){},
};

static void rootdev_init() {
  device_attach(&rootdev);
}

SYSINIT_ADD(rootdev, rootdev_init, DEPS("mount_fs"));
