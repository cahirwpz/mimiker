#include <sys/mimiker.h>
#include <riscv/riscvreg.h>

__no_profile void cpu_intr_disable(void) {
  csr_clear(sstatus, SSTATUS_SIE);
}

__no_profile void cpu_intr_enable(void) {
  csr_set(sstatus, SSTATUS_SIE);
}

__no_profile bool cpu_intr_disabled(void) {
  register_t sstatus = csr_read(sstatus);
  return (sstatus & SSTATUS_SIE) == 0;
}
