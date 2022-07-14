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
 *   - temporarily mapping kernel page directory into VM (because there is
 *     no direct map to access it in the usual fashion),
 *
 *   - Preparing KASAN shadow memory for the mapped area,
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
#include <sys/boot.h>
#include <sys/fdt.h>
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <sys/pmap.h>
#include <riscv/abi.h>
#include <riscv/cpufunc.h>
#include <riscv/pmap.h>
#include <libfdt/libfdt.h>

#define PHYSADDR(x) ((paddr_t)((vaddr_t)(x) + (KERNEL_PHYS - KERNEL_VIRT)))
#define VIRTADDR(x) ((vaddr_t)((paddr_t)(x) + (KERNEL_VIRT - KERNEL_PHYS)))

#define BOOT_KASAN_SANITIZED_SIZE(end)                                         \
  roundup2(roundup2((intptr_t)end, GROWKERNEL_STRIDE) - KASAN_SANITIZED_START, \
           PAGESIZE * KASAN_SHADOW_SCALE_SIZE)

#define BOOT_PD_VADDR (DMAP_BASE + GROWKERNEL_STRIDE)

/*
 * Virtual memory boot data.
 */

/* NOTE: the boot stack is used before we switch out to `thread0`. */
static alignas(STACK_ALIGN) uint8_t boot_stack[PAGESIZE];

/*
 * Bare memory boot data.
 */

static __noreturn void riscv_boot(void *dtb, paddr_t pde, paddr_t bootmem_end,
                                  vaddr_t kernel_end);

/* Without `volatile` Clang applies constant propagation optimization and
 * that ends up generating relocations in `.text` instead of `.data` section.
 * This is exactly what we're trying to avoid here! */
__boot_data static volatile vaddr_t _text = (vaddr_t)__text;
__boot_data static volatile vaddr_t _data = (vaddr_t)__data;
__boot_data static volatile vaddr_t _bss = (vaddr_t)__bss;
__boot_data static volatile vaddr_t _ebss = (vaddr_t)__ebss;
__boot_data static volatile vaddr_t _riscv_boot = (vaddr_t)riscv_boot;
__boot_data static volatile vaddr_t _boot_stack = (vaddr_t)boot_stack;

__boot_text static pde_t *early_pde_ptr(pde_t *pde, int lvl, vaddr_t va) {
  if (lvl == 0)
    return pde + L0_INDEX(va);
#if __riscv_xlen == 32
  return pde + L1_INDEX(va);
#else
  if (lvl == 1)
    return pde + L1_INDEX(va);
  return pde + L2_INDEX(va);
#endif
}

__boot_text static pte_t *early_ensure_pte(pde_t *pde, vaddr_t va) {
  pde_t *pdep = early_pde_ptr(pde, 0, va);

  for (unsigned lvl = 1; lvl < PAGE_TABLE_DEPTH; lvl++) {
    paddr_t pa;
    if (!VALID_PDE_P(*pdep)) {
      pa = (paddr_t)boot_sbrk(PAGESIZE);
      *pdep = PA_TO_PTE(pa) | PTE_V;
    } else {
      pa = (paddr_t)PTE_TO_PA(*pdep);
    }
    pdep = early_pde_ptr((pde_t *)pa, lvl, va);
  }

  return (pte_t *)pdep;
}

__boot_text static void early_kenter(pde_t *pde, vaddr_t va, size_t size,
                                     paddr_t pa, u_long flags) {
  if (!is_aligned(size, PAGESIZE))
    halt();

  for (size_t off = 0; off < size; off += PAGESIZE) {
    pte_t *ptep = early_ensure_pte(pde, va + off);
    *ptep = PA_TO_PTE(pa + off) | flags;
  }
}

__boot_text static pde_t *build_page_table(vaddr_t kernel_end) {
  /* Allocate kernel page directory.*/
  pde_t *pde = boot_sbrk(PAGESIZE);

  /* Kernel read-only segment - sections: .text and .rodata. */
  early_kenter(pde, _text, _data - _text, PHYSADDR(_text), PTE_X | PTE_KERN_RO);

  /* Kernel read-write segment - sections: .data and .bss. */
  early_kenter(pde, _data, kernel_end - _data, PHYSADDR(_data), PTE_KERN);

  /*
   * NOTE: we don't have to map the boot allocation area as the allocated
   * data will only be accessed using physical addresses (see pmap).
   */

  /* Kernel page directory table. */
  early_kenter(pde, BOOT_PD_VADDR, PAGESIZE, (paddr_t)pde, PTE_KERN);

#if KASAN
  size_t shadow_size =
    BOOT_KASAN_SANITIZED_SIZE(kernel_end) / KASAN_SHADOW_SCALE_SIZE;
  paddr_t shadow_mem = (paddr_t)boot_sbrk(shadow_size);

  early_kenter(pde, KASAN_SHADOW_START, shadow_size, shadow_mem, PTE_KERN);
#endif /* !KASAN */

  return pde;
}

__boot_text __noreturn void riscv_init(paddr_t dtb) {
  boot_clear(PHYSADDR(_bss), PHYSADDR(_ebss));
  boot_sbrk_init(PHYSADDR(_ebss));

  vaddr_t dtb_va = VIRTADDR(boot_save_dtb(dtb));

  /* Make sure DTB is mapped into kernel virtual address space. */
  vaddr_t kernel_end = VIRTADDR(boot_sbrk_align(PAGESIZE));

  /* Build kernel page table. */
  pde_t *pde = build_page_table(kernel_end);

  _bootmem_end = boot_sbrk(0);

  /* Temporarily set the trap vector. */
  csr_write(stvec, _riscv_boot);

  /* Move to VM boot stage. */
#if __riscv_xlen == 64
  const paddr_t satp = SATP_MODE_SV39 | ((paddr_t)pde >> PAGE_SHIFT);
#else
  const paddr_t satp = SATP_MODE_SV32 | ((paddr_t)pde >> PAGE_SHIFT);
#endif

  vaddr_t boot_sp = _boot_stack + PAGESIZE;

  __sfence_vma();

  __asm __volatile("mv a0, %0\n\t"
                   "mv a1, %1\n\t"
                   "mv a2, %2\n\t"
                   "mv a3, %3\n\t"
                   "mv sp, %4\n\t"
                   "csrw satp, %5\n\t"
                   "sfence.vma\n\t"
                   "1: j 1b" /* triggers instruction fetch page fault */
                   :
                   : "r"(dtb_va), "r"(pde), "r"(_bootmem_end), "r"(kernel_end),
                     "r"(boot_sp), "r"(satp)
                   : "a0", "a1", "a2", "a3");
  __unreachable();
}

/*
 * Virtual memory boot functions.
 */

/* Trap handler in direct mode. */
extern void cpu_exception_handler(void);

extern void *board_stack(void);
extern void __noreturn board_init(void);

static void configure_cpu(void) {
  /* Set initial register values. */
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
}

static __noreturn void riscv_boot(void *dtb, paddr_t pde, paddr_t bootmem_end,
                                  vaddr_t kernel_end) {
  configure_cpu();

  vm_kernel_end = kernel_end;

  extern paddr_t kern_phys_end;
  kern_phys_end = bootmem_end;

#if KASAN
  _kasan_sanitized_end =
    KASAN_SANITIZED_START + BOOT_KASAN_SANITIZED_SIZE(__ebss);
#endif

  FDT_init(dtb);

  void *sp = board_stack();

  pmap_bootstrap(pde, (void *)BOOT_PD_VADDR);

  /*
   * Switch to thread0's stack and perform `board_init`.
   */
  __asm __volatile("mv sp, %0\n\t"
                   "tail board_init"
                   :
                   : "r"(sp));
  __unreachable();
}

/* TODO(MichalBlk): remove those after architecture split of dbg debug scripts.
 */
typedef struct {
} tlbentry_t;

static __used __boot_data volatile tlbentry_t _gdb_tlb_entry;
