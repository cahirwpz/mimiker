#include <sys/mimiker.h>
#include <aarch64/armreg.h>

/*
 * Enable hardware management of data coherency with other cores in the cluster.
 */
#define SMPEN (1 << 6)

#define __dsb(x) __asm__ volatile("DSB " x)
#define __isb() __asm__ volatile("ISB")

/* Return active CPU number [0-3]. */
__boot_text static long get_cpu(void) {
  return READ_SPECIALREG(MPIDR_EL1) & 3UL;
}

__boot_text static void enable_cache_coherency(void) {
  WRITE_SPECIALREG(S3_1_C15_c2_1, READ_SPECIALREG(S3_1_C15_C2_1) | SMPEN);
  __dsb("sy");
  __isb();
}

__boot_text void *aarch64_init(void) {
  long cpu = get_cpu();
  (void)cpu;

  enable_cache_coherency();
  for (;;)
    ;
}
