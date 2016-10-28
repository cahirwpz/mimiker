#include <malloc.h>
#include <mips/malta.h>

extern int _memsize;

void pm_boot() {
  pm_add_segment(MALTA_PHYS_SDRAM_BASE, MALTA_PHYS_SDRAM_BASE + _memsize,
                 MIPS_KSEG0_START);
  pm_reserve(MALTA_PHYS_SDRAM_BASE,
             (pm_addr_t)kernel_sbrk_shutdown() - MIPS_KSEG0_START);
}
