#include <mips/malta.h>
#include <bus.h>

static uint8_t cbus_read_1(bus_addr_t addr, off_t offset) {
  paddr_t pa = addr + offset * sizeof(uint64_t);
  return *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(pa);
}

static void cbus_write_1(bus_addr_t addr, off_t offset, uint8_t value) {
  paddr_t pa = addr + offset * sizeof(uint64_t);
  *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(pa) = value;
}

static bus_space_t cbus_bus_space = {
  .bs_read_1 = cbus_read_1, .bs_write_1 = cbus_write_1,
};

resource_t cbus_uart[1] = {{.r_bus_space = &cbus_bus_space,
                            .r_bus_addr = MALTA_CBUS_UART,
                            /* RT_MEMORY since CBUS UART is a device mapped in
                             * common physical address space of MIPS CPU. */
                            .r_type = RT_MEMORY,
                            .r_start = 0,
                            .r_end = 7}};
