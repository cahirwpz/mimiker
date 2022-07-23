#include <sys/boot.h>
#include <sys/fdt.h>
#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <sys/pmap.h>
#include <sys/kasan.h>
#include <aarch64/abi.h>
#include <aarch64/armreg.h>
#include <aarch64/vm_param.h>
#include <aarch64/pmap.h>

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

#define PHYSADDR(x) ((paddr_t)(x) & (~KERNEL_SPACE_BEGIN))
#define VIRTADDR(x) ((vaddr_t)(x) | KERNEL_SPACE_BEGIN)

#define BOOT_KASAN_SANITIZED_SIZE(end)                                         \
  roundup2(roundup2((vaddr_t)(end), PAGESIZE) - KASAN_SANITIZED_START,         \
           SUPERPAGESIZE * KASAN_SHADOW_SCALE_SIZE)

static __noreturn void aarch64_boot(void *dtb, paddr_t pde, paddr_t sbrk_end,
                                    vaddr_t vma_end);

static alignas(STACK_ALIGN) uint8_t _boot_stack[PAGESIZE];

extern char exception_vectors[];
extern char hypervisor_vectors[];

/* Without `volatile` Clang applies constant propagation optimization and
 * that ends up generating relocations in `.text` instead of `.data` section.
 * This is exactly what we're trying to avoid here! */
__boot_data static volatile paddr_t _boot = (paddr_t)__boot;
__boot_data static volatile paddr_t _eboot = (paddr_t)__eboot;
__boot_data static volatile vaddr_t _text = (vaddr_t)__text;
__boot_data static volatile vaddr_t _etext = (vaddr_t)__etext;
__boot_data static volatile vaddr_t _rodata = (vaddr_t)__rodata;
__boot_data static volatile vaddr_t _data = (vaddr_t)__data;
__boot_data static volatile vaddr_t _bss = (vaddr_t)__bss;
__boot_data static volatile vaddr_t _ebss = (vaddr_t)__ebss;
__boot_data static volatile vaddr_t _evec = (vaddr_t)exception_vectors;
__boot_data static volatile vaddr_t _hvec = (vaddr_t)hypervisor_vectors;
__boot_data static volatile vaddr_t _pcpu = (vaddr_t)_pcpu_data;

__boot_text static void configure_cpu(void) {
  /* Enable hw management of data coherency with other cores in the cluster. */
  WRITE_SPECIALREG(S3_1_C15_c2_1, READ_SPECIALREG(S3_1_C15_C2_1) | SMPEN);
  __dsb("sy");
  __isb();

  /* TODO(pj) update ASFLAGS in build system. */
#if 0
  /* We don't support ARMv8.2 Reliability, Availability, and Serviceability. */
  uint64_t pfr0 = READ_SPECIALREG(ID_AA64PFR0_EL1);
  if (ID_AA64PFR0_RAS_VAL(pfr0) != ID_AA64PFR0_RAS_NONE)
    halt();
#endif

  WRITE_SPECIALREG(tpidr_el1, _pcpu);
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
    WRITE_SPECIALREG(VBAR_EL2, _hvec);

    /* Prepare for jump into EL1. */
    WRITE_SPECIALREG(SP_EL1, __sp());
    WRITE_SPECIALREG(SPSR_EL2, PSR_DAIF | PSR_M_EL1h);
    WRITE_SPECIALREG(ELR_EL2, &&el1_entry);
    __isb();
    __eret();
  }

el1_entry:
  WRITE_SPECIALREG(SPSR_EL1, PSR_DAIF);
  return;
}

__boot_text static pde_t *early_pde_ptr(pde_t *pde, int lvl, vaddr_t va) {
  /* l0 entry is 512GB */
  if (lvl == 0)
    return pde + L0_INDEX(va);
  /* l1 entry is 1GB */
  if (lvl == 1)
    return pde + L1_INDEX(va);
  /* l2 entry is 2MB */
  if (lvl == 2)
    return pde + L2_INDEX(va);
  /* l3 entry is 4KB */
  return pde + L3_INDEX(va);
}

__boot_text static pte_t *early_ensure_pte(pde_t *pde, vaddr_t va) {
  pde_t *pdep = early_pde_ptr(pde, 0, va);

  for (unsigned lvl = 1; lvl < PAGE_TABLE_DEPTH; lvl++) {
    paddr_t pa;
    if (*pdep & Ln_VALID) {
      pa = (paddr_t)(*pdep) & L3_PAGE_OA;
    } else {
      pa = (paddr_t)boot_sbrk(PAGESIZE);
      *pdep = pa | L0_TABLE; /* works for all levels */
    }
    pdep = early_pde_ptr((pde_t *)pa, lvl, va);
  }

  return (pte_t *)pdep;
}

__boot_text static void early_kenter(pde_t *pde, vaddr_t va, vaddr_t va_end,
                                     paddr_t pa, u_long flags) {
  for (; va < va_end; va += PAGESIZE, pa += PAGESIZE) {
    pte_t *ptep = early_ensure_pte(pde, va);
    *ptep = pa | flags;
  }
}

/* Create direct map of whole physical memory located at DMAP_BASE virtual
 * address. We will use this mapping later in pmap module. */

__boot_text static pde_t *build_page_table(vaddr_t kernel_end) {
  /* Allocate kernel page directory.*/
  pde_t *pde = boot_sbrk(PAGESIZE);

  const pte_t pte_default =
    L3_PAGE | ATTR_AF | ATTR_SH_IS | ATTR_IDX(ATTR_NORMAL_MEM_WB);

  /* boot sections */
  early_kenter(pde, VIRTADDR(_boot), VIRTADDR(_eboot), _boot,
               ATTR_AP_RW | pte_default);

  /* text section */
  early_kenter(pde, _text, _etext, PHYSADDR(_text), ATTR_AP_RO | pte_default);

  /* rodata section */
  early_kenter(pde, _rodata, _data, PHYSADDR(_rodata),
               ATTR_AP_RO | ATTR_XN | pte_default);

  /* data & bss sections */
  early_kenter(pde, _data, _ebss, PHYSADDR(_data),
               ATTR_AP_RW | ATTR_XN | pte_default);

  /* direct map construction */
  early_kenter(pde, DMAP_BASE, DMAP_BASE + DMAP_SIZE, 0,
               ATTR_AP_RW | ATTR_XN | pte_default);

#if KASAN /* Prepare KASAN shadow mappings */
  size_t kasan_sanitized_size = BOOT_KASAN_SANITIZED_SIZE(_ebss);
  size_t kasan_shadow_size = kasan_sanitized_size / KASAN_SHADOW_SCALE_SIZE;
  /* Allocate physical memory for shadow area */
  paddr_t kasan_shadow_pa = (paddr_t)boot_sbrk(kasan_shadow_size);

  early_kenter(pde, KASAN_SHADOW_START, KASAN_SHADOW_START + kasan_shadow_size,
               kasan_shadow_pa, ATTR_AP_RW | ATTR_XN | pte_default);
#endif /* KASAN */

  return pde;
}

/* Based on locore.S from FreeBSD. */
__boot_text static void enable_mmu(pde_t *pde) {
  __dsb("sy");

  WRITE_SPECIALREG(VBAR_EL1, _evec);
  WRITE_SPECIALREG(TTBR0_EL1, pde);
  WRITE_SPECIALREG(TTBR1_EL1, pde);
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
   * M   - MMU enable for EL1 and EL0 stage 1 address translation.
   * I   - Cacheability control.
   * C   - Cacheability control, for data accesses.
   * A   - Alignment check.
   * SA  - SP alignment check - EL1.
   * SA0 - SP alignment check - EL0.
   *
   */
  WRITE_SPECIALREG(sctlr_el1, SCTLR_M | SCTLR_I | SCTLR_C | SCTLR_A | SCTLR_SA |
                                SCTLR_SA0);
  __isb();
}

__boot_text __noreturn void aarch64_init(paddr_t dtb) {
  drop_to_el1();
  configure_cpu();
  boot_clear(PHYSADDR(_bss), PHYSADDR(_ebss));
  boot_sbrk_init(PHYSADDR(_ebss));

  vaddr_t dtb_va = VIRTADDR(boot_save_dtb(dtb));

  /* Make sure DTB is mapped into kernel virtual address space. */
  vaddr_t vma_end = VIRTADDR(boot_sbrk_align(PAGESIZE));

  pde_t *pde = build_page_table(vma_end);
  enable_mmu(pde);

  void *sbrk_end = boot_sbrk(0);

  vaddr_t boot_sp = (vaddr_t)&_boot_stack[PAGESIZE];

  __asm __volatile("mov x0, %0\n\t"
                   "mov x1, %1\n\t"
                   "mov x2, %2\n\t"
                   "mov x3, %3\n\t"
                   "mov sp, %4\n\t"
                   "br %5"
                   :
                   : "r"(dtb_va), "r"(pde), "r"(sbrk_end), "r"(vma_end),
                     "r"(boot_sp), "r"(aarch64_boot)
                   : "x0", "x1", "x2", "x3");
  __unreachable();
}

extern void *board_stack(void);

static __noreturn void aarch64_boot(void *dtb, paddr_t pde, paddr_t sbrk_end,
                                    vaddr_t vma_end) {
  vm_kernel_end = vma_end;
  boot_sbrk_end = sbrk_end;

#if KASAN
  _kasan_sanitized_end =
    KASAN_SANITIZED_START + BOOT_KASAN_SANITIZED_SIZE((vaddr_t)__ebss);
#endif

  FDT_init(dtb);

  void *sp = board_stack();

  pmap_bootstrap(pde, (pde_t *)(pde + DMAP_BASE));

  /*
   * Switch to thread0's stack and perform `board_init`.
   */
  __asm __volatile("mov sp, %0\n\t"
                   "b board_init"
                   :
                   : "r"(sp));
  __unreachable();
}

/* TODO(pj) Remove those after architecture split of gdb debug scripts. */
typedef struct {
} tlbentry_t;

static __used __boot_data volatile tlbentry_t _gdb_tlb_entry;
