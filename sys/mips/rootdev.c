#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <mips/malta.h>
#include <mips/mips.h>
#include <mips/interrupt.h>
#include <sys/bus.h>
#include <sys/exception.h>
#include <sys/pci.h>
#include <sys/sysinit.h>
#include <sys/devclass.h>

typedef struct rootdev {
  void *data;
} rootdev_t;

/* TODO: remove following lines when devclasses are implemented */
extern pci_bus_driver_t gt_pci_bus;
device_t *gt_pci;

static rman_t rm_mem; /* stores all resources of root bus children */

static void rootdev_intr_setup(device_t *dev, unsigned num,
                               intr_handler_t *handler) {
  mips_intr_setup(handler, num);
}

static void rootdev_intr_teardown(device_t *dev, intr_handler_t *handler) {
  mips_intr_teardown(handler);
}

static int rootdev_attach(device_t *dev) {
  /* Manages space occupied by I/O devices: PCI, FPGA, system controler, ... */
  rman_init(&rm_mem, "Malta I/O space", 0x10000000, 0x1fffffff, RT_MEMORY);

  gt_pci = device_add_child(dev);
  gt_pci->driver = &gt_pci_bus.driver;
  if (device_probe(gt_pci))
    device_attach(gt_pci);
  return 0;
}

static int rootdev_bs_map(bus_addr_t addr, bus_size_t size,
                          bus_space_handle_t *handle_p) {
  *handle_p = MIPS_PHYS_TO_KSEG1(addr);
  return 0;
}

/* clang-format off */
bus_space_t *rootdev_bus_space = &(bus_space_t){
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

static resource_t *rootdev_alloc_resource(device_t *bus, device_t *child,
                                          res_type_t type, int rid,
                                          rman_addr_t start, rman_addr_t end,
                                          size_t size, res_flags_t flags) {

  resource_t *r =
    rman_alloc_resource(&rm_mem, start, end, size, 1, RF_NONE, child);

  if (r) {
    r->r_bus_tag = rootdev_bus_space;
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

static device_t rootdev = (device_t){
  .children = TAILQ_HEAD_INITIALIZER(rootdev.children),
  .driver = (driver_t *)&rootdev_driver,
  .instance = &(rootdev_t){},
  .state = NULL,
};

static void rootdev_init(void) {
  device_attach(&rootdev);
}

SYSINIT_ADD(rootdev, rootdev_init, DEPS("mount_fs", "ithread"));
DEVCLASS_CREATE(root);
