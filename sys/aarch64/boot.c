#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <aarch64/armreg.h>
#include <aarch64/vm_param.h>
#include <aarch64/pmap.h>
#include <aarch64/pte.h>
#include <aarch64/atags.h>
#include <aarch64/kasan.h>

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

/* Last physical address used by kernel for boot memory allocation. */
__boot_data void *_bootmem_end;
/* Kernel page directory entries. */
paddr_t _kernel_pmap_pde;
alignas(PAGESIZE) uint8_t _atags[PAGESIZE];
static alignas(PAGESIZE) uint8_t _boot_stack[PAGESIZE];

extern char exception_vectors[];
extern char hypervisor_vectors[];
#if KASAN
extern size_t _kasan_sanitized_end;
__boot_data static vaddr_t kasan_sanitized_end;
#endif

__boot_text static void halt(void) {
  for (;;)
    continue;
}

/* Allocates & clears pages in physical memory just after kernel image end. */
static __boot_text void *bootmem_alloc(size_t bytes) {
  uint64_t *addr = _bootmem_end;
  _bootmem_end += bytes;
  for (unsigned i = 0; i < bytes / sizeof(uint64_t); i++)
    addr[i] = 0;
  return addr;
}

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

  WRITE_SPECIALREG(tpidr_el1, _pcpu_data);
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

#define AARCH64_PHYSADDR(x) ((paddr_t)(x) & (~KERNEL_SPACE_BEGIN))

__boot_text static void clear_bss(void) {
  uint64_t *ptr = (uint64_t *)AARCH64_PHYSADDR(__bss);
  uint64_t *end = (uint64_t *)AARCH64_PHYSADDR(__ebss);
  while (ptr < end)
    *ptr++ = 0;
}

/* Create direct map of whole physical memory located at DMAP_BASE virtual
 * address. We will use this mapping later in pmap module. */

/* XXX Raspberry PI 3 specific! */
#define DMAP_SIZE 0x3c000000
#define DMAP_BASE 0xffffff8000000000 /* last 512GB */

#define DMAP_L3_ENTRIES max(1, DMAP_SIZE / PAGESIZE)
#define DMAP_L2_ENTRIES max(1, DMAP_L3_ENTRIES / PT_ENTRIES)
#define DMAP_L1_ENTRIES max(1, DMAP_L2_ENTRIES / PT_ENTRIES)

#define DMAP_L1_SIZE roundup(DMAP_L1_ENTRIES * sizeof(pde_t), PAGESIZE)
#define DMAP_L2_SIZE roundup(DMAP_L2_ENTRIES * sizeof(pde_t), PAGESIZE)
#define DMAP_L3_SIZE roundup(DMAP_L3_ENTRIES * sizeof(pte_t), PAGESIZE)

#define PTE_MASK 0xfffffffffffff000
#define PTE_FRAME_ADDR(pte) ((pte)&PTE_MASK)

__boot_text static paddr_t build_page_table(void) {
  /* l0 entry is 512GB */
  volatile pde_t *l0 = bootmem_alloc(PAGESIZE);
  /* l1 entry is 1GB */
  volatile pde_t *l1 = bootmem_alloc(PAGESIZE);
  /* l2 entry is 2MB */
  volatile pde_t *l2 = bootmem_alloc(PAGESIZE);
  /* l3 entry is 4KB */
  volatile pte_t *l3 = bootmem_alloc(PAGESIZE);

  paddr_t text = AARCH64_PHYSADDR(__text);
  paddr_t data = AARCH64_PHYSADDR(__data);
  paddr_t ebss = AARCH64_PHYSADDR(roundup((vaddr_t)__ebss, PAGESIZE));
  vaddr_t va = KERNEL_SPACE_BEGIN + (vaddr_t)__boot;

  l0[L0_INDEX(va)] = (pde_t)l1 | L0_TABLE;
  l1[L1_INDEX(va)] = (pde_t)l2 | L1_TABLE;
  l2[L2_INDEX(va)] = (pde_t)l3 | L2_TABLE;

  /* TODO(pj) imitate pmap_growkernel from NetBSD */
  l2[L2_INDEX(0)] = (pde_t)bootmem_alloc(PAGESIZE) | L2_TABLE;
  for (int i = 0; i < 32; i++) {
    l2[L2_INDEX(0xffff000000400000 + i * PAGESIZE * PT_ENTRIES)] =
      (pde_t)bootmem_alloc(PAGESIZE) | L2_TABLE;
  }

  const pte_t pte_default =
    L3_PAGE | ATTR_AF | ATTR_SH_IS | ATTR_IDX(ATTR_NORMAL_MEM_WB);

  paddr_t pa = (paddr_t)__boot;

  /* boot sections */
  for (; pa < text; pa += PAGESIZE, va += PAGESIZE)
    l3[L3_INDEX(va)] = pa | ATTR_AP_RW | pte_default;

  /* text section */
  for (; pa < data; pa += PAGESIZE, va += PAGESIZE)
    l3[L3_INDEX(va)] = pa | ATTR_AP_RO | pte_default;

  /* data & bss sections */
  for (; pa < ebss; pa += PAGESIZE, va += PAGESIZE)
    l3[L3_INDEX(va)] = pa | ATTR_AP_RW | ATTR_XN | pte_default;

  /* direct map construction */
  volatile pde_t *l1d = bootmem_alloc(DMAP_L1_SIZE);
  volatile pde_t *l2d = bootmem_alloc(DMAP_L2_SIZE);
  volatile pde_t *l3d = bootmem_alloc(DMAP_L3_SIZE);

  for (intptr_t i = 0; i < DMAP_L3_ENTRIES; i++)
    l3d[i] = (i * PAGESIZE) | ATTR_AP_RW | ATTR_XN | pte_default;

  for (intptr_t i = 0; i < DMAP_L2_ENTRIES; i++)
    l2d[i] = (pde_t)&l3d[i * PT_ENTRIES] | L2_TABLE;

  for (intptr_t i = 0; i < DMAP_L1_ENTRIES; i++)
    l1d[i] = (pde_t)&l2d[i * PT_ENTRIES] | L1_TABLE;

  l0[L0_INDEX(DMAP_BASE)] = (pde_t)l1d | L0_TABLE;

#if KASAN /* Prepare KASAN shadow mappings */
  kasan_sanitized_end =
    SUPERPAGESIZE + roundup(va, SUPERPAGESIZE << KASAN_SHADOW_SCALE_SHIFT);
  size_t kasan_sanitized_size = kasan_sanitized_end - KASAN_MD_SANITIZED_START;
  size_t kasan_shadow_size =
    roundup(kasan_sanitized_size >> KASAN_SHADOW_SCALE_SHIFT, SUPERPAGESIZE);
  vaddr_t kasan_shadow_end = KASAN_MD_SHADOW_START + kasan_shadow_size;
  va = KASAN_MD_SHADOW_START;
  /* Allocate physical memory for shadow area */
  pa = (paddr_t)bootmem_alloc(kasan_shadow_size);

  while (va < kasan_shadow_end) {
    if (l0[L0_INDEX(va)] == 0)
      l0[L0_INDEX(va)] = (pde_t)bootmem_alloc(PAGESIZE) | L0_TABLE;

    pde_t *l1k = (pde_t *)PTE_FRAME_ADDR(l0[L0_INDEX(va)]);
    if (l1k[L1_INDEX(va)] == 0)
      l1k[L1_INDEX(va)] = (pde_t)bootmem_alloc(PAGESIZE) | L1_TABLE;

    pde_t *l2k = (pde_t *)PTE_FRAME_ADDR(l1k[L1_INDEX(va)]);
    if (l2k[L2_INDEX(va)] == 0)
      l2k[L2_INDEX(va)] = (pde_t)bootmem_alloc(PAGESIZE) | L2_TABLE;

    pde_t *l3k = (pde_t *)PTE_FRAME_ADDR(l2k[L2_INDEX(va)]);

    for (int j = 0; j < PT_ENTRIES; j++) {
      l3k[L3_INDEX(va)] = pa | ATTR_AP_RW | ATTR_XN | pte_default;
      va += PAGESIZE;
      pa += PAGESIZE;
    }
  }
#endif /* !KASAN */

  return (paddr_t)l0;
}

/* Based on locore.S from FreeBSD. */
__boot_text static void enable_mmu(paddr_t pde) {
  __dsb("sy");

  WRITE_SPECIALREG(VBAR_EL1, exception_vectors);
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

  _kernel_pmap_pde = pde;
#if KASAN
  _kasan_sanitized_end = kasan_sanitized_end;
#endif
}

__boot_text static void atags_copy(atag_tag_t *atags) {
  uint8_t *dst = (uint8_t *)AARCH64_PHYSADDR(_atags);

  atag_tag_t *tag;
  ATAG_FOREACH(tag, atags) {
    size_t size = ATAG_SIZE(tag);
    uint8_t *src = (uint8_t *)tag;
    for (size_t i = 0; i < size; i++)
      *dst++ = *src++;
  }
}

__boot_text void *aarch64_init(atag_tag_t *atags) {
  drop_to_el1();
  configure_cpu();
  clear_bss();

  /* Set end address of kernel for boot allocation purposes. */
  _bootmem_end = (void *)align(AARCH64_PHYSADDR(__ebss), PAGESIZE);
  atags_copy(atags);

  enable_mmu(build_page_table());
  return _atags;
}

void *aarch64_init_boot_stack(void) {
  return &_boot_stack[PAGESIZE];
}

/* TODO(pj) Remove those after architecture split of gdb debug scripts. */
typedef struct {
} tlbentry_t;

static __unused __boot_data volatile tlbentry_t _gdb_tlb_entry;
