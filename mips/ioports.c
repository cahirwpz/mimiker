#include <mips/malta.h>
#include <bus.h>

static uint8_t mips_ioports_read_1(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_start + offset;
  return *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr);
}

static void mips_ioports_write_1(resource_t *handle, unsigned offset,
                                 uint8_t value) {
  intptr_t addr = handle->r_start + offset;
  *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr) = value;
}

static uint16_t mips_ioports_read_2(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_start + offset;
  return *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(addr);
}

static void mips_ioports_write_2(resource_t *handle, unsigned offset,
                                 uint16_t value) {
  intptr_t addr = handle->r_start + offset;
  *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(addr) = value;
}

static bus_space_t mips_ioports_bus_space = {.read_1 = mips_ioports_read_1,
                                             .write_1 = mips_ioports_write_1,
                                             .read_2 = mips_ioports_read_2,
                                             .write_2 = mips_ioports_write_2};

/* For compatibility reasons, PCI specification reserves some part of the low
   I/O range for use by ISA components (PCI->ISA bridge). */
#define IOPORTS_BASE 0x18000000

resource_t ioports = {.r_bus_space = &mips_ioports_bus_space,
                      .r_type = RT_IOPORTS,
                      .r_start = 0x0000 + IOPORTS_BASE,
                      .r_end = 0xffff + IOPORTS_BASE};
