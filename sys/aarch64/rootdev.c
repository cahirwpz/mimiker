#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <aarch64/bcm2835reg.h>
#include <sys/kmem.h>
#include <sys/pmap.h>

typedef struct rootdev {
  rman_t local_rm;
  rman_t shared_rm;
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
  /* TODO(pj) */
}

static void rootdev_intr_teardown(device_t *dev, intr_handler_t *handler) {
  /* TODO(pj) */
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
  rootdev_local_handle = kva_alloc(BCM2836_ARM_LOCAL_SIZE);
  pmap_kenter(rootdev_local_handle, BCM2836_ARM_LOCAL_BASE,
              VM_PROT_READ | VM_PROT_WRITE, PMAP_NOCACHE);

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
    device_add_resource(child, r, rid);
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
