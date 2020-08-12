#include <sys/mimiker.h>
#include <aarch64/armreg.h>
#include <aarch64/vm_param.h>
#include <aarch64/pmap.h>
#include <aarch64/pte.h>

#define __tlbi(x) __asm__ volatile("TLBI " x)
#define __dsb(x) __asm__ volatile("DSB " x)
#define __isb() __asm__ volatile("ISB")
#define __eret() __asm__ volatile("ERET")
#define __sp()                                                                 \
  ({                                                                           \
    uint64_t __rv;                                                             \
    __asm __volatile("mov %0, sp" : "=r"(__rv));                               \
    __rv;                                                                      \
  })

/* Last address used by kernel for boot allocation. */
__boot_data void *_kernel_end_boot;
/* Kernel page directory entries. */
alignas(PAGESIZE) pte_t _kernel_pmap_pde[PD_ENTRIES];
/* The boot stack is used before we switch out to thread0. */
__boot_data alignas(PAGESIZE) uint8_t _boot_stack[PAGESIZE];

extern char exception_vectors[];
extern char hypervisor_vectors[];

__boot_text static void halt(void) {
  for (;;)
    continue;
}

/* Allocates pages. The argument will be aligned to PAGESIZE. */
static __boot_text void *bootmem_alloc(size_t bytes) {
  void *addr = _kernel_end_boot;
  _kernel_end_boot += align(bytes, PAGESIZE);
  return addr;
}

/* Enable hw management of data coherency with other cores in the cluster. */
__boot_text static void enable_cache_coherency(void) {
  WRITE_SPECIALREG(S3_1_C15_c2_1, READ_SPECIALREG(S3_1_C15_C2_1) | SMPEN);
  __dsb("sy");
  __isb();
}

__boot_text static void drop_to_el1(void) {
  uint64_t CurrentEl = READ_SPECIALREG(CurrentEl) >> 2;
  if (CurrentEl > 2) {
    /* --- Execution state control for lower Exception levels.
     * The next lower level is AArch64
     * --- Non-secure bit.
     * Indicates that Exception levels lower than EL3 are in Non-secure state,
     * and so memory accesses from those Exception levels cannot access Secure
     * memory.
     */
    WRITE_SPECIALREG(SCR_EL3, SCR_RW | SCR_NS);

    /* Prepare for jump into EL2. */
    WRITE_SPECIALREG(SP_EL2, __sp());
    WRITE_SPECIALREG(SPSR_EL3, PSR_DAIF | PSR_M_EL2h);
    WRITE_SPECIALREG(ELR_EL3, &&el2_entry);
    __isb();
    __eret();
  }

el2_entry:
  if (CurrentEl > 1) {
    /* The Execution state for EL1 is AArch64. The Execution state for EL0 is
     * determined by the current value of PSTATE.nRW when executing at EL0.
     */
    WRITE_SPECIALREG(HCR_EL2, HCR_RW);

    /* Load the Virtualization Process ID Register. */
    WRITE_SPECIALREG(VPIDR_EL2, READ_SPECIALREG(MIDR_EL1));

    /* Load the Virtualization Multiprocess ID Register. */
    WRITE_SPECIALREG(VMPIDR_EL2, READ_SPECIALREG(MPIDR_EL1));

    /* Don't trap to EL2 for CP15 traps */
    WRITE_SPECIALREG(HSTR_EL2, 0);

    /* Enable access to the physical timers at EL1. */
    uint64_t cnthctl = READ_SPECIALREG(CNTHCTL_EL2);
    cnthctl |= CNTHCTL_EL1PCTEN | CNTHCTL_EL1PCEN;
    WRITE_SPECIALREG(CNTHCTL_EL2, cnthctl);

    /* Set the counter offset to a known value */
    WRITE_SPECIALREG(CNTVOFF_EL2, 0);

    /* Hypervisor trap functions. */
    WRITE_SPECIALREG(VBAR_EL2, hypervisor_vectors);

    /* Prepare for jump into EL1. */
    WRITE_SPECIALREG(SP_EL1, __sp());
    WRITE_SPECIALREG(SPSR_EL2, PSR_DAIF | PSR_M_EL1h);
    WRITE_SPECIALREG(ELR_EL2, &&el1_entry);
    __isb();
    __eret();
  }

el1_entry:
  return;
}

extern char __boot[];
extern char __text[];
extern char __data[];
extern char __bss[];
extern char __ebss[];

#define AARCH64_PHYSADDR(x) ((paddr_t)(x) & (~KERNEL_SPACE_BEGIN))

__boot_text static void clear_bss(void) {
  uint64_t *ptr = (uint64_t *)AARCH64_PHYSADDR(__bss);
  uint64_t *end = (uint64_t *)AARCH64_PHYSADDR(__ebss);
  while (ptr < end)
    *ptr++ = 0;
}

__boot_text static void build_page_table(void) {
  /* l0 entry is 512GB */
  volatile pde_t *l0 = (pde_t *)AARCH64_PHYSADDR(_kernel_pmap_pde);
  /* l1 entry is 1GB */
  volatile pde_t *l1 = bootmem_alloc(PAGESIZE);
  /* l2 entry is 2MB */
  volatile pde_t *l2 = bootmem_alloc(PAGESIZE);
  /* l3 entry is 4KB */
  volatile pte_t *l3 = bootmem_alloc(PAGESIZE);

  for (int i = 0; i < PD_ENTRIES; i++) {
    l0[i] = 0;
    l1[i] = 0;
    l2[i] = 0;
    l3[i] = 0;
  }

  paddr_t text = AARCH64_PHYSADDR(__text);
  paddr_t data = AARCH64_PHYSADDR(__data);
  paddr_t ebss = AARCH64_PHYSADDR(roundup((vaddr_t)__ebss, PAGESIZE));
  vaddr_t va = KERNEL_SPACE_BEGIN + (vaddr_t)__boot;

  l0[L0_INDEX(va)] = (pde_t)l1 | L0_TABLE;
  l1[L1_INDEX(va)] = (pde_t)l2 | L1_TABLE;
  l2[L2_INDEX(va)] = (pde_t)l3 | L2_TABLE;

  const pte_t pte_default =
    L3_PAGE | ATTR_AF | ATTR_SH(ATTR_SH_IS) | ATTR_IDX(ATTR_NORMAL_MEM_WB);

  paddr_t pa = (paddr_t)__boot;

  /* initial stack :( */
  l3[0] = ATTR_AP(ATTR_AP_RW) | ATTR_XN | pte_default;

  /* boot sections */
  for (; pa < text; pa += PAGESIZE, va += PAGESIZE)
    l3[L3_INDEX(va)] = pa | ATTR_AP(ATTR_AP_RW) | pte_default;

  /* text section */
  for (; pa < data; pa += PAGESIZE, va += PAGESIZE)
    l3[L3_INDEX(va)] = pa | ATTR_AP(ATTR_AP_RO) | pte_default;

  /* data & bss sections */
  for (; pa < ebss; pa += PAGESIZE, va += PAGESIZE)
    l3[L3_INDEX(va)] = pa | ATTR_AP(ATTR_AP_RW) | ATTR_XN | pte_default;
}

/* Based on locore.S from FreeBSD. */
__boot_text static void enable_mmu(void) {
  __dsb("sy");

  WRITE_SPECIALREG(VBAR_EL1, exception_vectors);
  WRITE_SPECIALREG(TTBR0_EL1, AARCH64_PHYSADDR(_kernel_pmap_pde));
  WRITE_SPECIALREG(TTBR1_EL1, AARCH64_PHYSADDR(_kernel_pmap_pde));
  __isb();

  /* Clear the Monitor Debug System control register. */
  WRITE_SPECIALREG(MDSCR_EL1, 0);

  __tlbi("vmalle1is");

  /* Define memory attributes that will be used in page descriptors. */
  uint64_t mair = MAIR_ATTR(MAIR_DEVICE_nGnRnE, ATTR_DEVICE_MEM) |
                  MAIR_ATTR(MAIR_NORMAL_NC, ATTR_NORMAL_MEM_NC) |
                  MAIR_ATTR(MAIR_NORMAL_WB, ATTR_NORMAL_MEM_WB) |
                  MAIR_ATTR(MAIR_NORMAL_WT, ATTR_NORMAL_MEM_WT);
  WRITE_SPECIALREG(MAIR_EL1, mair);

  uint64_t mmfr0 = READ_SPECIALREG(id_aa64mmfr0_el1);
  uint64_t mmfr1 = READ_SPECIALREG(id_aa64mmfr1_el1);

  /* CPU must support 16 bits ASIDs. */
  if (ID_AA64MMFR0_ASIDBits_VAL(mmfr0) != ID_AA64MMFR0_ASIDBits_16)
    halt();

  /* CPU must support 4kB granules. */
  if (ID_AA64MMFR0_TGran4_VAL(mmfr0) != ID_AA64MMFR0_TGran4_IMPL)
    halt();

  /* Let's assume that the hardware doesn't support updates to Access flag and
   * Dirty state in translation tables. */
  if (ID_AA64MMFR1_HAFDBS_VAL(mmfr1) != ID_AA64MMFR1_HAFDBS_NONE)
    halt();

  /* Copy Intermediate Physical Address Size. */
  uint64_t tcr = ID_AA64MMFR0_PARange_VAL(mmfr0) << TCR_IPS_SHIFT;
  /* Use 16 bits ASIDs. */
  tcr |= TCR_ASID_16;
  /* Set user & kernel address space to have 2^48 addresses. */
  tcr |= TCR_T0SZ(16ULL) | TCR_T1SZ(16ULL);
  /* Set user & kernel granule to have 4kB. */
  tcr |= TCR_TG0_4K | TCR_TG1_4K;
  /* How TTBRx page tables will be cached (write-back write-allocate). */
  tcr |= TCR_IRGN0_WBWA | TCR_IRGN1_WBWA | TCR_ORGN0_WBWA | TCR_ORGN1_WBWA;
  /* How TTBR0 & TTBR1 page tables will be synchronized between CPUs. */
  tcr |= TCR_SH0_IS | TCR_SH1_IS;
  WRITE_SPECIALREG(tcr_el1, tcr);

  /* --- more magic bits
   * M -- MMU enable for EL1 and EL0 stage 1 address translation.
   * I -- SP must be aligned to 16.
   * C -- Cacheability control, for data accesses.
   */
  WRITE_SPECIALREG(sctlr_el1, SCTLR_M | SCTLR_I | SCTLR_C);
  __isb();
}

__boot_text void *aarch64_init(__unused void *atags) {
  drop_to_el1();
  enable_cache_coherency();
  clear_bss();

  /* Set end address of kernel for boot allocation purposes. */
  _kernel_end_boot = (void *)align(AARCH64_PHYSADDR(__ebss), PAGESIZE);

  build_page_table();
  enable_mmu();

  return NULL;
}
