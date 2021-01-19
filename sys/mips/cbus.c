#include <mips/malta.h>
#include <mips/mips.h>
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
resource_t cbus_uart[1] = {{.r_bus_tag = &cbus_bus_space,
                            .r_bus_handle = 0,
			   }};
/* clang-format on */
