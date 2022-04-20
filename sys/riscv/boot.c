/*
 * RISC-V kernel boot process.
 *
 * The boot process is divided into two parts:
 *  - Bare memory part contained in the boot segment (realized by `riscv_init`),
 *  - Virtual memory part (realized by `riscv_boot`).
 *
 *  The sole goal of the boot segment is to bring the kernel to the virtual
 *  address space. This involves the following tasks:
 *   - mapping kernel image (.text, .rodata, .data, .bss) into VM,
 *
 *   - temporarily mapping DTB into VM (as we will need to access it in the
 *     second stage of the boot process and we can't create the direct map
 *     before knowing the physical memory boundaries which are contained in the
 *     DTB itself),
 *
 *   - temoprarily mapping kernel page directory into VM (because there is
 *     no direct map to access it in the usual fashion),
 *
 *   - preparing page tables for `vm_page_t` structs (we need to do so
 *     because the procedure that maps kernel virtual space allocated for the
 *     structs can't allocate physical pages on its own since
 *     the buddy system isn't initialized at that point),
 *
 *   - enabling MMU and moving to the second stage.
 *
 * Scheme used for the transition from the first stage to the second stage:
 *   - we don't map the boot segment into VM,
 *
 *   - we assume boot (physical) addresses don't overlap with the mapped image
 *     (thereby an attempt to fetch the first instruction
 *     after enabling MMU will resault in an instruction fetch page fault),
 *
 *   - we temporarily set the trap vector to the address of `riscv_boot`,
 *
 *   - before enabling MMU we:
 *       - set parameters for `riscv_boot`,
 *       - set SP to VM boot stack.
 *
 * The second stage performs all operations needed to accomplish
 * board initialization. This includes:
 *   - setting registers to obey kernel conventions:
 *       - thread pointer register (`$tp`) always points to the PCPU structure
 *         of the hart,
 *       - SSCRARCH register always contains 0 when the hart operates in
 *         supervisor mode, and kernel stack pointer while in user mode.
 *
 *   - setting trap vector to trap handling routine,
 *
 *   - clearing any pending supervisor interrupts and disabling them afterwards,
 *
 *   - preparing bss,
 *
 *   - processing borad stack (this includes kernel environment setting),
 *
 *   - performing pmap bootstrap (this includes creating dmap),
 *
 *   - moving to thread0's stack and advancing to board initialization.
 *
 * For initial mapping of DTB and kernel PD the dmap area is used
 * as it will be overwritten in `pmap_bootstrap` where the temporary mappings
 * will be dropped.
 */
#define KL_LOG KL_INIT
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <riscv/abi.h>
#include <riscv/cpufunc.h>
#include <riscv/pmap.h>
#include <riscv/pte.h>
#include <riscv/vm_param.h>

#define KERNEL_IMG_END align(RISCV_PHYSADDR(_ebss), PAGESIZE)
#define KERNEL_PHYS_END (KERNEL_IMG_END + BOOTMEM_SIZE)

#define BOOT_DTB_VADDR DMAP_VADDR_BASE
#define BOOT_PD_VADDR (DMAP_VADDR_BASE + L0_SIZE)

/*
 * Bare memory boot data.
 */

/* Last physical address used by kernel for boot memory allocation. */
__boot_data static void *bootmem_brk;

/* End of boot memory allocation area. */
__boot_data static void *bootmem_end;

/* Boot symbols. */
__boot_data static vaddr_t _kernel_start;
__boot_data static vaddr_t _text;
__boot_data static vaddr_t _riscv_boot;
__boot_data static vaddr_t _data;
__boot_data static uint8_t *_boot_stack;
__boot_data static vaddr_t _ebss;
__boot_data static vaddr_t _kernel_end;
__boot_data static size_t _kernel_size;

/*
 * Virtual memory boot data.
 */

/* NOTE: the boot stack is used before we switch out to thread0. */
#define __bss_boot_stack __section(".bss.boot_stack")
static __bss_boot_stack
  __used alignas(STACK_ALIGN) uint8_t boot_stack[PAGESIZE];

/* Kernel symbols. */
paddr_t _eboot;

/*
 * Bare memory boot functions.
 */

__boot_text static __noreturn void halt(void) {
  for (;;)
    __wfi();
}

__boot_text static void bootmem_init(void) {
  bootmem_brk = (void *)KERNEL_IMG_END;
  bootmem_end = (void *)KERNEL_PHYS_END;
}

/*
 * Allocate and clear pages in physical memory just after kernel image end.
 * The argument will be aligned to `PAGESIZE`.
 */
__boot_text static void *bootmem_alloc(size_t bytes) {
  long *addr = bootmem_brk;
  bootmem_brk += align(bytes, PAGESIZE);

  if (bootmem_brk > bootmem_end)
    halt();

  for (size_t i = 0; i < bytes / sizeof(long); i++)
    addr[i] = 0;
  return addr;
}

__boot_text static void set_boot_syms(void) {
#if __riscv_xlen == 64
  extern char __boot_syms[];
  uintptr_t *syms = (uintptr_t *)__boot_syms;
  _kernel_start = (vaddr_t)syms[1];
  _text = (vaddr_t)syms[2];
  _riscv_boot = (vaddr_t)syms[3];
  _data = (vaddr_t)syms[4];
  _boot_stack = (uint8_t *)syms[5];
  _ebss = (vaddr_t)syms[6];
  _kernel_end = (vaddr_t)syms[7];
  _kernel_size = (size_t)syms[8];
#else
  _kernel_start = (vaddr_t)__kernel_start;
  _text = (vaddr_t)__text;
  _riscv_boot = (vaddr_t)riscv_boot;
  _data = (vaddr_t)__data;
  _boot_stack = (uint8_t *)boot_stack;
  _ebss = (vaddr_t)__ebss;
  _kernel_end = (vaddr_t)__kernel_end;
  _kernel_size = (size_t)__kernel_size;
#endif
}

__boot_text static void *alloc_pts(void) {
  /* Allocate kernel page directories.*/
  pd_entry_t *l0_pde = bootmem_alloc(PAGESIZE);

#if __riscv_xlen == 64
  pd_entry_t *l1_pde = bootmem_alloc(PAGESIZE);
  const size_t l0_idx = L0_INDEX(_text);
  l0_pde[l0_idx] = PA_TO_PTE((paddr_t)l1_pde) | PTE_V;
#endif

  /*
   * !HACK!
   * See 4th point of bare memory boot description
   * at the top of this file for details.
   */
#if __riscv_xlen == 64
  const size_t idx = L1_INDEX(_text);
  pd_entry_t *pde = l1_pde;
#else
  const size_t idx = L0_INDEX(_text);
  pd_entry_t *pde = l0_pde;
#endif

  for (int i = 0; i < VM_PAGE_PDS; i++) {
    paddr_t pt = (paddr_t)bootmem_alloc(PAGESIZE);
    pde[idx + i + 1] = PA_TO_PTE(pt) | PTE_V;
  }

  return l0_pde;
}

/* NOTE: `pde` points at the most inner PD. */
__boot_text static void map_kernel_image(pd_entry_t *img_pde) {
  /* Assume that kernel image will be covered by a single page table. */
#if __xlen_riscv == 64
  if (_kernel_size > L1_SIZE)
    halt();
#else
  if (_kernel_size > L0_SIZE)
    halt();
#endif

  pt_entry_t *pte = bootmem_alloc(PAGESIZE);

  /* Set appropriate page directory entry. */
#if __riscv_xlen == 64
  const size_t pde_idx = L1_INDEX(_text);
#else
  const size_t pde_idx = L0_INDEX(_text);
#endif
  img_pde[pde_idx] = PA_TO_PTE((paddr_t)pte) | PTE_V;

  /*
   * Map successive kernel segments.
   */
  const paddr_t data = RISCV_PHYSADDR(_data);
  const paddr_t ebss = KERNEL_IMG_END;

  vaddr_t va = _text;
  paddr_t pa = RISCV_PHYSADDR(va);

  /* Read-only segment - sections: .text and .rodata. */
  for (; pa < data; pa += PAGESIZE, va += PAGESIZE)
    pte[PTE_INDEX(va)] = PA_TO_PTE(pa) | PTE_X | PTE_KERN_RO;

  /* Read-write segment - sections: .data and .bss. */
  for (; pa < ebss; pa += PAGESIZE, va += PAGESIZE)
    pte[PTE_INDEX(va)] = PA_TO_PTE(pa) | PTE_KERN;

  /*
   * NOTE: we don't have to map the boot allocation area as the allocated
   * data will only be accessed using physical addresses (see pmap).
   */
}

__boot_text static void map_dtb(paddr_t dtb, pd_entry_t *l0_pde) {
  const size_t l0_idx = L0_INDEX(BOOT_DTB_VADDR);
  pd_entry_t *l1_pde = bootmem_alloc(PAGESIZE);
  l0_pde[l0_idx] = PA_TO_PTE((paddr_t)l1_pde) | PTE_V;

#if __riscv_xlen == 64
  pt_entry_t *l2_pte = bootmem_alloc(PAGESIZE);
  *l1_pde = PA_TO_PTE((paddr_t)l2_pte) | PTE_V;
#endif

#if __riscv_xlen == 64
  pt_entry_t *dtb_pte = l2_pte;
#else
  pt_entry_t *dtb_pte = (pt_entry_t *)l1_pde;
#endif

  paddr_t pa = dtb;
  for (int i = 0; i < Ln_ENTRIES; i++, pa += PAGESIZE)
    dtb_pte[i] = PA_TO_PTE(pa) | PTE_KERN;
}

__boot_text static void map_pd(pd_entry_t *l0_pde) {
  const size_t l0_idx = L0_INDEX(BOOT_PD_VADDR);
  pd_entry_t *l1_pde = bootmem_alloc(PAGESIZE);
  l0_pde[l0_idx] = PA_TO_PTE((paddr_t)l1_pde) | PTE_V;

#if __riscv_xlen == 64
  pt_entry_t *l2_pte = bootmem_alloc(PAGESIZE);
  *l1_pde = PA_TO_PTE((paddr_t)l2_pte) | PTE_V;
#endif

#if __riscv_xlen == 64
  pt_entry_t *pd_pte = l2_pte;
#else
  pt_entry_t *pd_pte = (pt_entry_t *)l1_pde;
#endif

  *pd_pte = PA_TO_PTE((paddr_t)l0_pde) | PTE_KERN;
}

#define __text_riscv_boot __section(".text.riscv_boot")
static __text_riscv_boot __used __noreturn void riscv_boot(paddr_t dtb,
                                                           paddr_t pde);

__boot_text __noreturn void riscv_init(paddr_t dtb) {
  set_boot_syms();

  if (!((paddr_t)__eboot < _kernel_start || _kernel_end < (paddr_t)__boot))
    halt();

  bootmem_init();

  /* Create kernel page directory. */
  pd_entry_t *l0_pde = alloc_pts();

#if __riscv_xlen == 64
  const size_t img_idx = L0_INDEX(_text);
  pd_entry_t *img_pde = (pd_entry_t *)PTE_TO_PA(l0_pde[img_idx]);
#else
  pd_entry_t *img_pde = l0_pde;
#endif

  map_kernel_image(img_pde);
  map_dtb(dtb, l0_pde);
  map_pd(l0_pde);

  /* Temporarily set the trap vector. */
  csr_write(stvec, _riscv_boot);

  /*
   * Move to VM boot stage.
   */
#if __riscv_xlen == 64
  const paddr_t satp = SATP_MODE_SV39 | ((paddr_t)l0_pde >> PAGE_SHIFT);
#else
  const paddr_t satp = SATP_MODE_SV32 | ((paddr_t)l0_pde >> PAGE_SHIFT);
#endif
  void *boot_sp = &_boot_stack[PAGESIZE];

  __sfence_vma();

  __asm __volatile("mv a0, %0\n\t"
                   "mv a1, %1\n\t"
                   "mv sp, %2\n\t"
                   "csrw satp, %3\n\t"
                   "nop" /* triggers instruction fetch page fault */
                   :
                   : "r"(dtb), "r"(l0_pde), "r"(boot_sp), "r"(satp)
                   : "a0", "a1");

  __unreachable();
}

/*
 * Virtual memory boot functions.
 */

static void set_kernel_syms(void) {
#if __riscv_xlen == 64
  extern char __kernel_syms[];
  uintptr_t *syms = (uintptr_t *)__kernel_syms;
  _eboot = (paddr_t)syms[0];
#else
  _eboot = (paddr_t)__eboot;
#endif
}

static void clear_bss(void) {
  long *ptr = (long *)__bss;
  long *end = (long *)__ebss;
  while (ptr < end)
    *ptr++ = 0;
}

/* Trap handler in direct mode. */
extern void cpu_exception_handler(void);

extern void *board_stack(paddr_t dtb_pa, vaddr_t dtb_va);
extern void __noreturn board_init(void);

static __text_riscv_boot void riscv_boot(paddr_t dtb, paddr_t l0_pde) {
  /*
   * Set initial register values.
   */
  __set_tp();
  csr_write(sscratch, 0);

  /*
   * Set trap vector base address:
   *  - MODE = Direct - all exceptions set PC to specified BASE
   */
  csr_write(stvec, cpu_exception_handler);

  /*
   * NOTE: respective interrupts will be enabled by appropriate device drivers
   * while registering an interrupt handling routine.
   */
  csr_clear(sie, SIP_SEIP | SIP_STIP | SIP_SSIP);
  csr_clear(sie, SIE_SEIE | SIE_STIE | SIE_SSIE);

  set_kernel_syms();

  clear_bss();

  void *sp = board_stack(dtb, BOOT_DTB_VADDR);

  pmap_bootstrap(l0_pde, BOOT_PD_VADDR);

  /*
   * Switch to thread0's stack and perform `board_init`.
   */
  __asm __volatile("mv sp, %0\n\t"
                   "tail board_init" ::"r"(sp));
  __unreachable();
}

/* TODO(MichalBlk): remove those after architecture split of dbg debug scripts.
 */
typedef struct {
  int _pad;
} tlbentry_t;

static __used __boot_data volatile tlbentry_t _gdb_tlb_entry;
