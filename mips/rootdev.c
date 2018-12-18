#define KL_LOG KL_DEV
#include <klog.h>
#include <mips/malta.h>
#include <mips/intr.h>
#include <bus.h>
#include <exception.h>
#include <pci.h>
#include <sysinit.h>
#include <devclass.h>

/* typedef struct rootdev { */
/*   void *data; */
/* } rootdev_softc_t; */

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

  // bus_hinted_child(dev);
  device_add_nameunit_child(dev, "pci", 0); // this device should be added by bus_hinted_child(dev) !
  bus_generic_attach(dev);
  return 0;
}

static int rootdev_bs_map(bus_addr_t addr, bus_size_t size, int flags,
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
                  BUS_SPACE_MAP_LINEAR, &r->r_bus_handle);
    device_add_resource(child, r, rid);
  }

  return r;
}

/* clang-format off */
static bus_driver_t rootdev_driver = {
  .driver =
    {
      .size = NULL,
      .desc = "MIPS platform root bus driver",
      .attach = rootdev_attach,
    },
  .bus = {
    .intr_setup = rootdev_intr_setup,
    .intr_teardown = rootdev_intr_teardown,
    .alloc_resource = rootdev_alloc_resource
  }
};
/* clang-format on */

// should I move this to rootdev_init?
// or maybe should this remain as static init?
static device_t rootdev = (device_t){
  .parent = NULL,
  /* .all ?? */
  /* .link ?? */
  .children = TAILQ_HEAD_INITIALIZER(rootdev.children),
  /* .resources ?? */
  .name = "root",
  .unit = 0,
  /* .bus ?? */
  .driver = (driver_t *)&rootdev_driver,
  .ivars = NULL,
  .softc = NULL
};

static void rootdev_init(void) {
  device_attach(&rootdev);
}

// this is the only driver added to sysinit
SYSINIT_ADD(rootdev, rootdev_init, DEPS("mount_fs"));
DEVCLASS_CREATE(root);
