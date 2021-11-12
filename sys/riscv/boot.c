#include <sys/fdt.h>
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <riscv/abi.h>
#include <riscv/board.h>
#include <riscv/pmap.h>
#include <riscv/riscvreg.h>
#include <riscv/vm_param.h>

#define BOOT_DTB_VADDR DMAP_VADDR_BASE
#define BOOT_PD_VADDR KERNEL_SPACE_BEGIN

#define __wfi() __asm__ volatile("wfi")
#define __set_tp() __asm__ volatile("mv tp, %0" ::"r"(_pcpu_data) : "tp")
#define __sfence_vma() __asm__ volatile("sfence.vma" ::: "memory")

/*
 * NOTE: the sole goal of the boot segment is to bring the kernel
 * to the virtual address space. The boot sections themselves mustn't be mapped
 * along with the kernel.
 */

/*
 * Bare memory data.
 */

/* Last physical address used by kernel for boot memory allocation. */
__boot_data static void *bootmem_brk;

/* End of boot memory allocation area. */
__boot_data static void *bootmem_end;

/*
 * Virtual memory data.
 */

/* The boot stack is used before we switch out to thread0. */
static alignas(STACK_ALIGN) uint8_t boot_stack[PAGESIZE];

/*
 * Bare memory boot functions.
 */

__boot_text static __noreturn void halt(void) {
  for (;;) {
    __wfi();
  }
}

__boot_text static void bootmem_init(void) {
  bootmem_brk = (void *)align((paddr_t)__ebss, PAGESIZE);
  bootmem_end = bootmem_brk + BOOTMEM_SIZE;
}

/* Allocate and clear pages in physical memory just after kernel image end.
 * The argument will be aligned to `PAGESIZE`. */
__boot_text static void *bootmem_alloc(size_t bytes) {
  long *addr = bootmem_brk;
  bootmem_brk += align(bytes, PAGESIZE);

  if (bootmem_brk > bootmem_end)
    halt();

  for (size_t i = 0; i < bytes / sizeof(long); i++)
    addr[i] = 0;
  return addr;
}

__boot_text static void map_kernel_image(pd_entry_t *pde) {
  /* Assume that kernel image will be covered by single PDE (4MiB). */
  pt_entry_t *pte = bootmem_alloc(PAGESIZE);

  /* Set appropriate page directory entry. */
  size_t idx = L0_INDEX((vaddr_t)__text);
  pde[idx] = PA_TO_PTE((paddr_t)pte) | PTE_V | PTE_G;

  /*
   * Map successive kernel segments.
   */
  paddr_t data = RISCV_PHYSADDR(__data);
  paddr_t ebss = RISCV_PHYSADDR(align((vaddr_t)__ebss, PAGESIZE));

  vaddr_t va = (vaddr_t)__text;
  paddr_t pa = RISCV_PHYSADDR(va);

  /* Read-only segment - sections: .text and .rodata. */
  for (; pa < data; pa += PAGESIZE, va += PAGESIZE)
    pte[L1_INDEX(va)] = PA_TO_PTE(pa) | PTE_X | PTE_KERN_RO;

  /* Read-write segment - sections: .data and .bss. */
  for (; pa < ebss; pa += PAGESIZE, va += PAGESIZE)
    pte[L1_INDEX(pa)] = PA_TO_PTE(pa) | PTE_KERN;

  /*
   * NOTE: we don't have to map the boot allocation area as the allocated
   * data will only be accessed using physical addresses (see pmap).
   */
}

/*
 * NOTE: before we can create a direct map we need to know the memory boundaries
 * which are included in the dtb. Thereby, we need to temporarily map the dtb
 * into VM before initializing the dtb module.
 */
__boot_text static void map_dtb(paddr_t dtb, pd_entry_t *pde) {
  /* Assume that dbt will be covered by single superpage (4MiB). */
  size_t idx = L0_INDEX(BOOT_DTB_VADDR);
  pde[idx] = PA_TO_PTE(dtb) | PTE_KERN;
}

/*
 * NOTE: in order to access kernel PD from `riscv_boot` we need to temporarily
 * map it into VM.
 */
__boot_text static void map_pd(pd_entry_t *pde) {
  /* Kernel PD will be mapped before `__kernel_start` through the same PT
   * as the kernel. */
  size_t pd_idx = L0_INDEX(BOOT_PD_VADDR);
  pt_entry_t *pte = (pt_entry_t *)PTE_TO_PA(pde[pd_idx]);
  size_t pt_idx = L1_INDEX(BOOT_PD_VADDR);
  pte[pt_idx] = PA_TO_PTE((paddr_t)pde) | PTE_KERN;
}

static __noreturn void riscv_boot(paddr_t dtb, paddr_t pd);

__boot_text __noreturn void riscv_init(paddr_t dtb) {
  /*
   * NOTE: for our soultion to work, boot physical addresses mustn't
   * overlap with kernel virtual addresses.
   */
  if (!(__eboot < __kernel_start || __kernel_end < __boot))
    halt();

  bootmem_init();

  /* Create kernel page directory. */
  pd_entry_t *pde = bootmem_alloc(PAGESIZE);

  map_kernel_image(pde);
  map_dtb(dtb, pde);
  map_pd(pde);

  /*
   * NOTE: the first instruction fetch after enabling MMU will cause
   * the instruction page fault exception transfering us to the main
   * virtual memory boot function.
   */
  csr_write(stvec, riscv_boot);

  /*
   * 1. Set boot stack.
   * 2. Set boot parameters.
   * 3. Enable MMU:
   *     - MODE = Sv32 - Page-based 32-bit virtual addressing
   */
  register register_t t0 __asm("t0") =
    (register_t)(((paddr_t)pde >> PAGE_SHIFT) | SATP_MODE_SV32);
  register register_t sp __asm("sp") = (register_t)&boot_stack[PAGESIZE];
  register register_t a0 __asm("a0") = (register_t)dtb;
  register register_t a1 __asm("a1") = (register_t)pde;

  __sfence_vma();
  __asm __volatile("csrw satp, %0\n\t"
                   "nop" /* triggers instruction page fault */
                   :
                   : "r"(t0), "r"(sp), "r"(a0), "r"(a1));
  __unreachable();
}

/*
 * Virtual memory functions.
 */

static void clear_bss(void) {
  long *ptr = (long *)__bss;
  long *end = (long *)__ebss;
  while (ptr < end)
    *ptr++ = 0;
}

/* Trap handler in direct mode. */
void cpu_exception_handler(void);

static __noreturn void riscv_boot(paddr_t dtb, paddr_t pd) {
  /*
   * Set initial register values.
   */
  __set_tp();
  csr_write(sscratch, 0);

  /*
   * Set trap vector base address:
   *  - MODE = Direct - All exceptions set PC to BASE
   */
  csr_write(stvec, cpu_exception_handler);

  clear_bss();

  void *sp = board_stack(dtb, (void *)BOOT_DTB_VADDR);

  /* Initialize klog before bootstrapping pmap. */
  init_kasan();
  init_klog();

  pmap_bootstrap(pd, (pd_entry_t *)BOOT_PD_VADDR);

  /*
   * Switch to thread0's stack and perform `board_init`.
   */
  __asm __volatile("mv sp, %0\n\t"
                   "tail board_init" ::"r"(sp));
  __unreachable();
}

/* TODO(MichalBlk): remove those after architecture split of dbg debug
 * scripts. */
typedef struct {
} tlbentry_t;

static __unused __boot_data const volatile tlbentry_t _gdb_tlb_entry;
