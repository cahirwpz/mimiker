#include <mips/malta.h>
#include <bus.h>

static uint8_t cbus_read_1(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_start + offset * sizeof(uint64_t);
  return *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr);
}

static void cbus_write_1(resource_t *handle, unsigned offset, uint8_t value) {
  intptr_t addr = handle->r_start + offset * sizeof(uint64_t);
  *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr) = value;
}

static bus_space_t cbus_bus_space = {
  .read_1 = cbus_read_1, .write_1 = cbus_write_1,
};

resource_t cbus_uart[1] = {{.r_bus_space = &cbus_bus_space,
                            /* RT_MEMORY since CBUS UART is a device mapped in
                             * common physical address space of MIPS CPU. */
                            .r_type = RT_MEMORY,
                            .r_start = MALTA_CBUS_UART,
                            .r_end = MALTA_CBUS_UART + 7}};
