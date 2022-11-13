#include <sys/bus.h>

static uint8_t cbus_read_1(bus_space_handle_t handle, bus_size_t offset) {
  return *(volatile uint8_t *)(handle + offset * sizeof(uint64_t));
}

static void cbus_write_1(bus_space_handle_t handle, bus_size_t offset,
                         uint8_t value) {
  *(volatile uint8_t *)(handle + offset * sizeof(uint64_t)) = value;
}

static bus_space_t cbus_bus_space = {
  .bs_read_1 = cbus_read_1,
  .bs_write_1 = cbus_write_1,
};

/* clang-format off */
dev_mem_t cbus_uart[1] = {{.bus_tag = &cbus_bus_space,
                           .bus_handle = 0,
			  }};
/* clang-format on */
