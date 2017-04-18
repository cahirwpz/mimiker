#include <mips/malta.h>
#include <bus.h>

static uint8_t cbus_read_1(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_addr + offset * sizeof(uint64_t);
  return *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(addr));
}

static void cbus_write_1(resource_t *handle, unsigned offset, uint8_t value) {
  intptr_t addr = handle->r_addr + offset * sizeof(uint64_t);
  *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(addr)) = value;
}

static BUS_SPACE_DEFINE(cbus);

resource_t cbus_uart[1] = {{.r_bus_space = cbus_bus_space,
                            .r_addr = MALTA_CBUS_UART,
                            .r_start = 0,
                            .r_end = 7}};
