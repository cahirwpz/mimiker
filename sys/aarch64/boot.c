#include <sys/mimiker.h>
#include <aarch64/aarch64.h>
#include <aarch64/armreg.h>
#include <aarch64/vm_param.h>
#include <aarch64/pmap.h>
#include <aarch64/pte.h>

/*
 * Here we have only 62 bytes for stack per CPU. So we must be very carefully.
 */

#define AARCH64_PHYSADDR(x) ((paddr_t)(x)-KERNEL_SPACE_BEGIN)

/*
 * Enable hardware management of data coherency with other cores in the cluster.
 */
#define SMPEN (1 << 6)

#define __tlbi(x) __asm__ volatile("TLBI " x)
#define __dsb(x) __asm__ volatile("DSB " x)
#define __isb() __asm__ volatile("ISB")
#define __eret() __asm__ volatile("ERET")

#define REG_SP_READ()                                                          \
  ({                                                                           \
    uint64_t __rv;                                                             \
    __asm __volatile("mov %0, sp" : "=r"(__rv));                               \
    __rv;                                                                      \
  })

/* Last address used by kernel for boot allocation. */
__boot_data void *_kernel_end_boot;
/* Kernel page directory entries. */
alignas(PAGESIZE) pte_t _kernel_pmap_pde[PD_ENTRIES];

alignas(2048) char _exc_vector[2048];

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

__boot_text static void invalidate_tlb(void) {
  __tlbi("alle1");
  __tlbi("vmalle1is");
  __dsb("ish");
  __isb();
}

__boot_text static void clear_bss(void) {
  uint64_t *ptr = (uint64_t *)AARCH64_PHYSADDR(__bss);
  uint64_t *end = (uint64_t *)AARCH64_PHYSADDR(__ebss);
  while (ptr < end)
    *ptr++ = 0;
}

/* Based on locore.S from FreeBSD. */
__boot_text static void enable_mmu(void) {
  __dsb("sy");
  __isb();

  WRITE_SPECIALREG(VBAR_EL1, _exc_vector);
  WRITE_SPECIALREG(TTBR0_EL1, _kernel_pmap_pde);
  WRITE_SPECIALREG(TTBR1_EL1, _kernel_pmap_pde);

  invalidate_tlb();

  /* Define memory attributes that will be used in page descriptors. */
  uint64_t mair = MAIR_ATTR(MAIR_DEVICE_nGnRnE, ATTR_DEVICE_MEM) |
                  MAIR_ATTR(MAIR_NORMAL_NC, ATTR_NORMAL_MEM_NC) |
                  MAIR_ATTR(MAIR_NORMAL_WB, ATTR_NORMAL_MEM_WB) |
                  MAIR_ATTR(MAIR_NORMAL_WT, ATTR_NORMAL_MEM_WT);
  WRITE_SPECIALREG(MAIR_EL1, mair);

  uint64_t mmfr0 = READ_SPECIALREG(id_aa64mmfr0_el1);
  uint64_t mmfr1 = READ_SPECIALREG(id_aa64mmfr1_el1);

  /* Copy Intermediate Physical Address Size. */
  uint64_t tcr = ID_AA64MMFR0_PARange_VAL(mmfr0) << TCR_IPS_SHIFT;

  /* CPU must support 16 bits ASIDs. */
  while ((mmfr0 & ID_AA64MMFR0_ASIDBits_MASK) != ID_AA64MMFR0_ASIDBits_16)
    continue;

  /* Let's assume that the hardware doesn't support updates to Access flag and
   * Dirty state in translation tables. */
  while ((mmfr1 & ID_AA64MMFR1_HAFDBS_MASK) != 0)
    continue;

  /* Use 16 bits ASIDs. */
  tcr |= TCR_ASID_16;
  /* Set user & kernel address space to have 2^48 addresses. */
  tcr |= TCR_T0SZ(16ULL) | TCR_T1SZ(16ULL);
  /* Set user & kernel granule to have 4kB. */
  tcr |= TCR_TG0_4K | TCR_TG1_4K;
  /* How TTBR0 & TTBR1 page tables will be cached (write-back write-allocate).
   */
  tcr |= TCR_IRGN0_WBWA | TCR_IRGN1_WBWA | TCR_ORGN0_WBWA | TCR_ORGN1_WBWA;
  /* How TTBR0 & TTBR1 page tables will be synchronized between CPUs. */
  tcr |= TCR_SH0_IS | TCR_SH1_IS;
  WRITE_SPECIALREG(tcr_el1, tcr);

  /* --- more magic bits
   * M -- MMU enable for EL1 and EL0 stage 1 address translation.
   * I -- SP must be aligned to 16.
   * C -- Cacheability control, for data accesses.
   */
  uint64_t sctlr = SCTLR_M | SCTLR_I | SCTLR_C;

  WRITE_SPECIALREG(sctlr_el1, sctlr);
  __dsb("sy");
  __isb();
}

__boot_text static void build_page_table(void) {
  /* l0 entry is 512GB */
  pde_t *l0 = _kernel_pmap_pde;
  /* l1 entry is 1GB */
  pde_t *l1 = bootmem_alloc(PAGESIZE);
  /* l2 entry is 2MB */
  pde_t *l2 = bootmem_alloc(PAGESIZE);
  /* l3 entry is 4KB */
  pde_t *l3 = bootmem_alloc(PAGESIZE);

  for (int i = 0; i < PD_ENTRIES; i++) {
    l0[i] = 0;
    l1[i] = 0;
    l2[i] = 0;
    l3[i] = 0;
  }

  paddr_t text = AARCH64_PHYSADDR(__text);
  paddr_t data = AARCH64_PHYSADDR(__data);
  paddr_t ebss = AARCH64_PHYSADDR(roundup((vaddr_t)__ebss, PAGESIZE));
  vaddr_t va = (vaddr_t)__text;

  l0[L0_INDEX(va)] = (pde_t)l1 | L0_TABLE;
  l1[L1_INDEX(va)] = (pde_t)l2 | L1_TABLE;
  l2[L2_INDEX(va)] = (pde_t)l3 | L2_TABLE;

  const pte_t pte_default =
    L3_PAGE | ATTR_SH(ATTR_SH_IS) | ATTR_IDX(ATTR_NORMAL_MEM_WB);

  paddr_t pa;
  for (pa = text; pa < data; pa += PAGESIZE, va += PAGESIZE)
    l3[L3_INDEX(va)] = pa | ATTR_AP(ATTR_AP_RO) | pte_default;

  for (; pa < ebss; pa += PAGESIZE, va += PAGESIZE)
    l3[L3_INDEX(va)] = pa | ATTR_AP(ATTR_AP_RW) | ATTR_XN | pte_default;
}

__boot_text void *aarch64_init(void) {
  long cpu = get_cpu();

  enable_cache_coherency();

  if (cpu != 0)
    for (;;)
      continue;

  clear_bss();
  /* Set end address of kernel for boot allocation purposes. */
  _kernel_end_boot = (void *)align(AARCH64_PHYSADDR(__ebss), PAGESIZE);

  build_page_table();
  enable_mmu();

  uint64_t x;
  /* --- el3_only */
  /* --- 0x8 = EL2 */
  if (READ_SPECIALREG(CurrentEl) != 0x8) {
    /* --- we are not in el2 */
    x = READ_SPECIALREG(SCR_EL3);
    /* --- Execution state control for lower Exception levels.
     * The next lower level is AArch64 */
    x |= (1 << 10);
    /* --- Non-secure bit.
     * Indicates that Exception levels lower than EL3 are in Non-secure state,
     * and so memory accesses from those Exception levels cannot access Secure
     * memory.
     */
    x |= (1 << 0);
    WRITE_SPECIALREG(SP_EL2, REG_SP_READ());
    WRITE_SPECIALREG(SCR_EL3, x);
    WRITE_SPECIALREG(SPSR_EL3, 0b01001);
    WRITE_SPECIALREG(ELR_EL3, &&el2_entry);
    __eret();
  }

el2_entry:
  WRITE_SPECIALREG(HCR_EL2, 0);
  /* The Execution state for EL1 is AArch64. The Execution state for EL0 is
   * determined by the current value of PSTATE.nRW when executing at EL0.
   */
  x = READ_SPECIALREG(HCR_EL2) | (1 << 31);
  WRITE_SPECIALREG(HCR_EL2, x);

  x = READ_SPECIALREG(CNTHCTL_EL2) | CNTHCTL_EL1PCTEN | CNTHCTL_EL1PCEN;
  /* --- enable timer for EL1 */
  WRITE_SPECIALREG(SP_EL1, REG_SP_READ());
  WRITE_SPECIALREG(CNTHCTL_EL2, x);
  WRITE_SPECIALREG(SPSR_EL2, 0b0101);
  WRITE_SPECIALREG(ELR_EL2, &&el1_entry);
  __eret();
el1_entry:

  return NULL;
}
