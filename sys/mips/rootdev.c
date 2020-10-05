#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <mips/malta.h>
#include <mips/mips.h>
#include <mips/interrupt.h>
#include <sys/bus.h>
#include <sys/exception.h>
#include <sys/pci.h>
#include <sys/devclass.h>

typedef struct rootdev {
  void *data;
} rootdev_t;

static rman_t rm_mem; /* stores all resources of root bus children */

static void rootdev_intr_setup(device_t *dev, unsigned num,
                               intr_handler_t *handler) {
  mips_intr_setup(handler, num);
}

static void rootdev_intr_teardown(device_t *dev, intr_handler_t *handler) {
  mips_intr_teardown(handler);
}

static int rootdev_attach(device_t *bus) {
  /* Manages space occupied by I/O devices: PCI, FPGA, system controler, ... */
  rman_init(&rm_mem, "Malta I/O space", 0x10000000, 0x1fffffff, RT_MEMORY);
  return bus_generic_probe(bus);
}

static resource_t *rootdev_alloc_resource(device_t *bus, device_t *child,
                                          res_type_t type, int rid,
                                          rman_addr_t start, rman_addr_t end,
                                          size_t size, res_flags_t flags) {

  resource_t *r =
    rman_alloc_resource(&rm_mem, start, end, size, 1, RF_NONE, child);

  if (r) {
    r->r_bus_tag = generic_bus_space;
    if (flags & RF_ACTIVE)
      bus_space_map(r->r_bus_tag, r->r_start, r->r_end - r->r_start + 1,
                    &r->r_bus_handle);
    device_add_resource(child, r, rid);
  }

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
          .alloc_resource = rootdev_alloc_resource}};

DEVCLASS_CREATE(root);

static device_t rootdev = (device_t){
  .children = TAILQ_HEAD_INITIALIZER(rootdev.children),
  .driver = (driver_t *)&rootdev_driver,
  .instance = &(rootdev_t){},
  .state = NULL,
  .devclass = &DEVCLASS(root),
};

void init_devices(void) {
  device_attach(&rootdev);
}
