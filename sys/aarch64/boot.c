#include <sys/mimiker.h>
#include <aarch64/aarch64.h>
#include <aarch64/armreg.h>
#include <aarch64/vm_param.h>

/*
 * Enable hardware management of data coherency with other cores in the cluster.
 */
#define SMPEN (1 << 6)

#define __tlbi(x) __asm__ volatile("TLBI " x)
#define __dsb(x) __asm__ volatile("DSB " x)
#define __isb() __asm__ volatile("ISB")

/* Last address used by kernel for boot allocation. */
__boot_data void *_kernel_end_boot;

/* Allocates pages. The argument will be aligned to PAGESIZE. */
__unused static __boot_text void *bootmem_alloc(size_t bytes) {
  void *addr = _kernel_end_boot;
  _kernel_end_boot += align(bytes, PAGESIZE);
  return addr;
}

/* Return active CPU number [0-3]. */
__boot_text static long get_cpu(void) {
  return READ_SPECIALREG(MPIDR_EL1) & 3UL;
}

__boot_text static void enable_cache_coherency(void) {
  WRITE_SPECIALREG(S3_1_C15_c2_1, READ_SPECIALREG(S3_1_C15_C2_1) | SMPEN);
  __dsb("sy");
  __isb();
}

__boot_text void invalidate_tlb(void) {
  __tlbi("alle1");
  __tlbi("vmalle1is");
  __dsb("ish");
  __isb();
}

__boot_text void *aarch64_init(void) {
  long cpu = get_cpu();

  enable_cache_coherency();
  invalidate_tlb();

  if (cpu != 0) {
    ;
  } else {
    _kernel_end_boot = (void *)align(AARCH64_PHYSADDR(__ebss), PAGESIZE);
  }

  for (;;)
    ;
}
