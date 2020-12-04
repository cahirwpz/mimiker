#include <mips/malta.h>
#include <mips/mips.h>
#include <sys/bus.h>
#include <sys/devclass.h>

typedef struct cbus_state {
  resource_t *cbus_memory;

  rman_t cbus_mem_rman;

} cbus_state_t;

DEVCLASS_CREATE(cbus);
DEVCLASS_DECLARE(root);

static uint8_t cbus_read_1(bus_space_handle_t handle, bus_size_t offset) {
  return *(volatile uint8_t *)(handle + offset * sizeof(uint64_t));
}

static void cbus_write_1(bus_space_handle_t handle, bus_size_t offset,
                         uint8_t value) {
  *(volatile uint8_t *)(handle + offset * sizeof(uint64_t)) = value;
}
// TODO: remove __unused
static bus_space_t cbus_bus_space __unused = {
  .bs_read_1 = cbus_read_1,
  .bs_write_1 = cbus_write_1,
};

static resource_t *cbus_alloc_resource(device_t *dev, res_type_t type __unused,
                                       int rid __unused, rman_addr_t start,
                                       rman_addr_t end, size_t size __unused,
                                       res_flags_t flags __unused) {

  // TODO
  // resource_t *res = rman_reserve_resource(dev->state)
  /*
    resource_alloc(NULL, start, end, flags);
  res->r_bus_tag = &cbus_bus_space;
  res->r_bus_handle = 0;
  return res;
  */
  return 0;
}

static int cbus_attach(device_t *cb) {
  cbus_state_t *cbs = cb->state;

  /* CBUS UART I/O memory */
  cbs->cbus_memory = bus_alloc_resource(cb, RT_MEMORY, 0, 0, 7, 0, 0);
  return 0;
}

static int cbus_probe(device_t *d) {
  return 1;
}

typedef struct cbus_driver {
  driver_t driver;
  bus_methods_t bus;
} cbus_driver_t;

cbus_driver_t cbus_driver = {
  .driver =
    {
      .desc = "CBUS Driver",
      .size = sizeof(cbus_state_t),
      .attach = cbus_attach,
      .probe = cbus_probe,
    },
  .bus =
    {
      .alloc_resource = cbus_alloc_resource,
    },
};

// TODO: remove this
/* clang-format off */
resource_t cbus_uart[1] = {{.r_bus_tag = &cbus_bus_space,
                            .r_bus_handle = 0,
			    .r_start = 0,
			    .r_end = 7}};
/* clang-format on */

DEVCLASS_ENTRY(root, cbus_driver);