#include <sys/mimiker.h>
#include <aarch64/armreg.h>

/* Return active CPU number [0-3]. */
__boot_text static long get_cpu(void) {
  return READ_SPECIALREG(MPIDR_EL1) & 3UL;
}

__boot_text void *aarch64_init(void) {
  long cpu = get_cpu();
  (void)cpu;
  for (;;)
    ;
}
