#include <mips/m32c0.h>
#include <mips/malta.h>
#include <mips/mips.h>
#include <mips/pmap.h>
#include <mips/tlb.h>
#include <sys/mimiker.h>
#include <sys/vm.h>
#include <mips/kasan.h>

/* Last address in kseg0 used by kernel for boot allocation. */
__boot_data void *_bootmem_end;
/* Pointer to kernel page directory allocated in kseg0. */
extern pde_t *_kernel_pmap_pde;
/* The boot stack is used before we switch out to thread0. */
static alignas(PAGESIZE) uint8_t _boot_stack[PAGESIZE];
extern size_t _kasan_sanitized_end;

/* Allocates pages in kseg0. The argument will be aligned to PAGESIZE. */
static __boot_text void *bootmem_alloc(size_t bytes) {
  void *addr = _bootmem_end;
  _bootmem_end += align(bytes, PAGESIZE);
  return addr;
}

__boot_text static void halt(void) {
  for (;;)
    continue;
}

__boot_text void *mips_init(void) {
  /*
   * Ensure we're in kernel mode, disable FPU,
   * leave error level & exception level and disable interrupts.
   */
  mips32_bc_c0(C0_STATUS,
               SR_IPL_MASK | SR_KSU_MASK | SR_CU1 | SR_ERL | SR_EXL | SR_IE);

  /* Clear pending software interrupts */
  mips32_bc_c0(C0_CAUSE, CR_IP_MASK);

  /*
   * Enable Vectored Interrupt Mode as described in „MIPS32® 24KETM Processor
   * Core Family Software User’s Manual”, chapter 6.3.1.2.
   */

  /* The location of exception vectors is set to EBase. */
  mips32_set_c0(C0_EBASE, _ebase);
  mips32_bc_c0(C0_STATUS, SR_BEV);
  /* Use the special interrupt vector at EBase + 0x200. */
  mips32_bs_c0(C0_CAUSE, CR_IV);
  /* Set vector spacing to 0. */
  mips32_set_c0(C0_INTCTL, INTCTL_VS_0);

  /* Clear BSS section using physical addresses. */
  uint32_t *ptr = (uint32_t *)MIPS_KSEG2_TO_KSEG0(__bss);
  uint32_t *end = (uint32_t *)MIPS_KSEG2_TO_KSEG0(__ebss);
  while (ptr < end)
    *ptr++ = 0;

  /* Set end address of kernel for boot allocation purposes. */
  _bootmem_end = (void *)align(MIPS_KSEG2_TO_KSEG0(__ebss), PAGESIZE);

  /* Clear all entries in TLB. */
  if ((mips32_getconfig0() & CFG0_MT_MASK) != CFG0_MT_TLB)
    halt();

  unsigned nentries =
    ((mips32_getconfig1() & CFG1_MMUS_MASK) >> CFG1_MMUS_SHIFT) + 1;

  mips32_setentryhi(0);
  mips32_setentrylo0(0);
  mips32_setentrylo1(0);
  for (unsigned i = 0; i < nentries; i++) {
    mips32_setindex(i);
    mips32_tlbwi();
  }

  /* Prepare 1:1 mapping between kseg2 and physical memory for kernel image. */
  pde_t *pde = (pde_t *)bootmem_alloc(PAGESIZE);
  for (int i = 0; i < PT_ENTRIES; i++)
    pde[i] = PTE_GLOBAL;

  pte_t *pte = (pte_t *)bootmem_alloc(PAGESIZE);
  for (int i = 0; i < PT_ENTRIES; i++)
    pte[i] = PTE_GLOBAL;

  paddr_t text = MIPS_KSEG2_TO_PHYS(__text);
  paddr_t data = MIPS_KSEG2_TO_PHYS(__data);
  paddr_t ebss = MIPS_KSEG2_TO_PHYS(roundup((vaddr_t)__ebss, PAGESIZE));
  vaddr_t va = MIPS_PHYS_TO_KSEG2(text);

  /* assume that kernel image will be covered by single PDE (4MiB) */
  pde[PDE_INDEX(va)] = PTE_PFN((paddr_t)pte) | PTE_KERNEL;

  /* read-only segment - sections: .text, .rodata, etc. */
  for (paddr_t pa = text; pa < data; va += PAGESIZE, pa += PAGESIZE)
    pte[PTE_INDEX(va)] = PTE_PFN(pa) | PTE_KERNEL_READONLY;

  /* read-write segment - sections: .data, .bss, etc. */
  for (paddr_t pa = data; pa < ebss; va += PAGESIZE, pa += PAGESIZE)
    pte[PTE_INDEX(va)] = PTE_PFN(pa) | PTE_KERNEL;

#if KASAN /* Prepare KASAN shadow mappings */
  /* The loop below where we map the shadow pages depends on
   * kasan_shadow_size % SUPERPAGESIZE == 0 for correctness. */
  size_t kasan_sanitized_size = roundup2(
    va - KASAN_MD_SANITIZED_START, SUPERPAGESIZE * KASAN_SHADOW_SCALE_SIZE);
  size_t kasan_shadow_size = kasan_sanitized_size / KASAN_SHADOW_SCALE_SIZE;
  va = KASAN_MD_SHADOW_START;
  /* Allocate physical memory for shadow area */
  paddr_t pa = (paddr_t)bootmem_alloc(kasan_shadow_size);
  /* How many PDEs should we use? */
  int num_pde = kasan_shadow_size / SUPERPAGESIZE;
  for (int i = 0; i < num_pde; i++) {
    /* Allocate a new PT */
    pte = bootmem_alloc(PAGESIZE);
    pde[PDE_INDEX(va)] = PTE_PFN((paddr_t)pte) | PTE_KERNEL;
    for (int j = 0; j < PT_ENTRIES; j++) {
      pte[PTE_INDEX(va)] = PTE_PFN(pa) | PTE_KERNEL;
      va += PAGESIZE;
      pa += PAGESIZE;
    }
  }
#endif /* !KASAN */

  /* 1st wired TLB entry is always occupied by kernel-PDE and user-PDE. */
  mips32_setwired(1);

  mips32_setentryhi(UPD_BASE);
  /* User root PDE is NULL */
  mips32_setentrylo0(PTE_GLOBAL);
  /* Kernel root PDE is set to pde */
  mips32_setentrylo1(PTE_PFN(MIPS_KSEG0_TO_PHYS(pde)) | PTE_KERNEL);
  mips32_setindex(0);
  mips32_tlbwi();

  /* Since variables are in kseg2 we cannot initialize them earlier. */
  _kernel_pmap_pde = pde;
#if KASAN
  _kasan_sanitized_end = KASAN_MD_SANITIZED_START + kasan_sanitized_size;
#endif /* !KASAN */

  /* Return the end of boot stack (grows downwards on MIPS) as new sp.
   * This is done in order to move kernel boot process to kseg2, since
   * current KASAN implementation requires all instrumented stack accesses
   * to be done through kseg2. */
  return &_boot_stack[PAGESIZE];
}

/* Following code is used by gdb scripts. */

/* Compiler does not know that debugger (external agent) will read
 * the structure and will remove it and optimize out all references to it.
 * Hence it has to be marked with `volatile`. */
static __boot_data volatile tlbentry_t _gdb_tlb_entry;
static __boot_data volatile asid_t _gdb_asid;

__boot_text unsigned _gdb_tlb_size(void) {
  return ((mips32_getconfig1() & CFG1_MMUS_MASK) >> CFG1_MMUS_SHIFT) + 1;
}

/* Fills _gdb_tlb_entry structure with TLB entry. */
__boot_text void _gdb_tlb_read_index(unsigned i) {
  tlbhi_t saved = mips32_getentryhi();
  _gdb_asid = saved & PTE_ASID_MASK;
  mips32_setindex(i);
  mips32_tlbr();
  _gdb_tlb_entry.hi = mips32_getentryhi();
  _gdb_tlb_entry.lo0 = mips32_getentrylo0();
  _gdb_tlb_entry.lo1 = mips32_getentrylo1();
  mips32_setentryhi(saved);
}
