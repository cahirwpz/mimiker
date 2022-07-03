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
#include <sys/console.h>
#include <sys/fdt.h>
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <sys/pmap.h>
#include <riscv/abi.h>
#include <riscv/cpufunc.h>
#include <riscv/pmap.h>

#define KERNEL_VIRT_IMG_END align((vaddr_t)__ebss, PAGESIZE)
#define KERNEL_PHYS_IMG_END align(RISCV_PHYSADDR(__ebss), PAGESIZE)

#define BOOT_KASAN_SANITIZED_SIZE                                              \
  roundup2(roundup2(KERNEL_VIRT_IMG_END, GROWKERNEL_STRIDE) -                  \
             KASAN_SANITIZED_START,                                            \
           PAGESIZE * KASAN_SHADOW_SCALE_SIZE)

#define BOOT_KASAN_SHADOW_SIZE                                                 \
  (BOOT_KASAN_SANITIZED_SIZE / KASAN_SHADOW_SCALE_SIZE)

#define BOOT_DTB_VADDR DMAP_BASE
#define BOOT_PD_VADDR (DMAP_BASE + GROWKERNEL_STRIDE)

/*
 * Bare memory boot data.
 */

/* Last physical address used by kernel for boot memory allocation. */
__boot_data static void *bootmem_brk;

__boot_data static pde_t *kernel_pde;

/*
 * Virtual memory boot data.
 */

/* NOTE: the boot stack is used before we switch out to `thread0`. */
static alignas(STACK_ALIGN) uint8_t boot_stack[PAGESIZE];

/*
 * Bare memory boot functions.
 */

__boot_text static __noreturn void halt(void) {
  for (;;)
    __wfi();
}

/*
 * Allocate and clear pages in physical memory just after kernel image end.
 * The argument will be aligned to `PAGESIZE`.
 */
__boot_text static void *bootmem_alloc(size_t bytes) {
  long *addr = bootmem_brk;
  bootmem_brk += align(bytes, PAGESIZE);

  for (size_t i = 0; i < bytes / sizeof(long); i++)
    addr[i] = 0;
  return addr;
}

__boot_text static pte_t *ensure_pte(vaddr_t va) {
  pde_t *pdep = kernel_pde;

  /* Level 0 */
  pdep += L0_INDEX(va);
  if (!VALID_PTE_P(*pdep))
    *pdep = PA_TO_PTE((paddr_t)bootmem_alloc(PAGESIZE)) | PTE_V;
  pdep = (pde_t *)PTE_TO_PA(*pdep);

  /* Level 1 */
  return (pte_t *)pdep + L1_INDEX(va);
}

__boot_text static void early_kenter(vaddr_t va, size_t size, paddr_t pa,
                                     u_long flags) {
  if (!is_aligned(size, PAGESIZE))
    halt();

  for (size_t off = 0; off < size; off += PAGESIZE) {
    pte_t *ptep = ensure_pte(va + off);
    *ptep = PA_TO_PTE(pa + off) | flags;
  }
}

static __noreturn void riscv_boot(paddr_t dtb, paddr_t pde, paddr_t kern_end);

__boot_text __noreturn void riscv_init(paddr_t dtb) {
  if (!((paddr_t)__eboot < (vaddr_t)__kernel_start ||
        (vaddr_t)__kernel_end < (paddr_t)__boot))
    halt();

  /* Initialize boot memory allocator. */
  bootmem_brk = (void *)KERNEL_PHYS_IMG_END;

  /* Allocate kernel page directory.*/
  kernel_pde = bootmem_alloc(PAGESIZE);

  /* Kernel read-only segment - sections: .text and .rodata. */
  early_kenter((vaddr_t)__text, __data - __text, RISCV_PHYSADDR(__text),
               PTE_X | PTE_KERN_RO);

  /* Kernel read-write segment - sections: .data and .bss. */
  early_kenter((vaddr_t)__data, KERNEL_VIRT_IMG_END - (vaddr_t)__data,
               RISCV_PHYSADDR(__data), PTE_KERN);

  /*
   * NOTE: we don't have to map the boot allocation area as the allocated
   * data will only be accessed using physical addresses (see pmap).
   */

  /* DTB - assume that DTB will be covered by a single last level page
   * directory. */
  early_kenter(BOOT_DTB_VADDR, GROWKERNEL_STRIDE, rounddown(dtb, PAGESIZE),
               PTE_KERN);

  /* Kernel page directory table. */
  early_kenter(BOOT_PD_VADDR, PAGESIZE, (paddr_t)kernel_pde, PTE_KERN);

#if KASAN
  paddr_t shadow_mem = (paddr_t)bootmem_alloc(BOOT_KASAN_SHADOW_SIZE);

  early_kenter(KASAN_SHADOW_START, BOOT_KASAN_SHADOW_SIZE, shadow_mem,
               PTE_KERN);
#endif /* !KASAN */

  /* Temporarily set the trap vector. */
  csr_write(stvec, riscv_boot);

  /*
   * Move to VM boot stage.
   */
  const paddr_t satp = SATP_MODE_SV32 | ((paddr_t)kernel_pde >> PAGE_SHIFT);
  void *boot_sp = &boot_stack[PAGESIZE];

  __sfence_vma();

  __asm __volatile("mv a0, %0\n\t"
                   "mv a1, %1\n\t"
                   "mv a2, %2\n\t"
                   "mv sp, %3\n\t"
                   "csrw satp, %4\n\t"
                   "sfence.vma\n\t"
                   "nop" /* triggers instruction fetch page fault */
                   :
                   : "r"(dtb), "r"(kernel_pde), "r"(bootmem_brk), "r"(boot_sp),
                     "r"(satp)
                   : "a0", "a1", "a2");

  __unreachable();
}

/*
 * Virtual memory boot functions.
 */

static void clear_bss(void) {
  long *ptr = (long *)__bss;
  long *end = (long *)__ebss;
  while (ptr < end)
    *ptr++ = 0;
}

/* Trap handler in direct mode. */
extern void cpu_exception_handler(void);

extern void *board_stack(paddr_t dtb_pa, void *dtb_va);
extern void __noreturn board_init(void);

static __noreturn void riscv_boot(paddr_t dtb, paddr_t pde, paddr_t kern_end) {
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

  clear_bss();

  init_cons();

  extern paddr_t kern_phys_end;
  kern_phys_end = kern_end;

#if KASAN
  _kasan_sanitized_end = KASAN_SANITIZED_START + BOOT_KASAN_SANITIZED_SIZE;
#endif

  void *dtb_va = (void *)BOOT_DTB_VADDR + (dtb & (PAGESIZE - 1));
  void *sp = board_stack(dtb, dtb_va);

  pmap_bootstrap(pde, (void *)BOOT_PD_VADDR);

  void *fdtp = phys_to_dmap(FDT_get_physaddr());
  FDT_changeroot(fdtp);

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
} tlbentry_t;

static __used __boot_data volatile tlbentry_t _gdb_tlb_entry;
