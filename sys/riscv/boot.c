/*
 * RISC-V kernel boot process.
 *
 * The boot process is divided into two parts:
 *  - Bare memory part contained in the boot segment (realized by `riscv_init`),
 *  - Virtual memory part (realized by `riscv_boot`).
 *
 *  The sole goal of the boot segment is to bring the kernel to the virtual
 *  address space. This involves the following tasks:
 *   - mapping kernel image (.text, .data, .bss) into VM,
 *
 *   - temporarily mapping dtb into VM (as we will need to access it in the
 *     second stage of the boot process and we can't create the direct map
 *     before knowing the physical memory boundaries which are contained in the
 *     dtb itself),
 *
 *   - temoprarily mapping kernel page directory into VM (because there is
 *     no direct map to access it in the usual fashion),
 *
 *   - preparing page tables for `vm_page_t` structs (we need to do so
 *     because the procedure that maps kernel virtual space allocated for the
 *     structs can't allocate pages on its own since buddy system isn't
 *     initialized at that point).
 *
 *   - enabling MMU and moving to the second stage.
 *
 * Scheme used for the transition from the first stage to the second stage:
 *   - we don't map the boot segment into VM,
 *
 *   - we assume boot addresses don't overlap with the mapped image
 *     (thereby the attempt to fetch the first instruction
 *     after enabling MMU will resault in an instruction fetch page fault),
 *
 *   - we temporarily set the trap vector to the address of `riscv_boot`,
 *
 *   - before enabling MMU we:
 *       - set parameters for `riscv_boot`,
 *       - set SP to VM boot stack,
 *
 * The second satge performs all operations needed to accomplish
 * board initialization. This includes:
 *   - setting registers to obey kernel conventions:
 *       - thread pointer register ($tp) always points to the PCPU structure
 *         of the hart,
 *       - SSCRARCH register always contains 0 when the hart operates in
 *         supervisor mode, and kernel stack pointer when in user mode.
 *
 *   - setting trap vector to kernel trap handling code,
 *
 *   - preparing bss,
 *
 *   - initializing kasan and klog,
 *
 *   - processing borad stack (this includes kernel environment setting),
 *
 *   - performing pmap bootstrap (this includes creating dmap),
 *
 *   - moving to thread0's stack and advancing to board initialization.
 *
 * For initial mapping of dtb and kernel pd the dmap area is used
 * as it will be overwritten in `pmap_bootstrap` thus the temporary mappings
 * will be dropped.
 */

#include <sys/fdt.h>
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <riscv/abi.h>
#include <riscv/pmap.h>
#include <riscv/riscvreg.h>
#include <riscv/vm_param.h>

#define BOOT_DTB_VADDR DMAP_VADDR_BASE
#define BOOT_MAX_DTB_SIZE L0_SIZE

#define BOOT_PD_VADDR (DMAP_VADDR_BASE + BOOT_MAX_DTB_SIZE)

#define __wfi() __asm__ volatile("wfi")
#define __set_tp() __asm__ volatile("mv tp, %0" ::"r"(_pcpu_data) : "tp")
#define __sfence_vma() __asm__ volatile("sfence.vma" ::: "memory")

/*
 * Bare memory boot data.
 */

/* Last physical address used by kernel for boot memory allocation. */
__boot_data static void *bootmem_brk;

/* End of boot memory allocation area. */
__boot_data static void *bootmem_end;

/*
 * Virtual memory boot data.
 */

/* NOTE: the boot stack is used before we switch out to thread0. */
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

__boot_text static void map_kernel_image(pd_entry_t *pde) {
  extern char __kernel_size[];

  /* Assume that kernel image will be covered by single PDE (4MiB). */
  assert((size_t)__kernel_size <= L0_SIZE);

  pt_entry_t *pte = bootmem_alloc(PAGESIZE);

  /* Set appropriate page directory entry. */
  const size_t idx = L0_INDEX((vaddr_t)__text);
  pde[idx] = PA_TO_PTE((paddr_t)pte) | PTE_V;

  /*
   * Map successive kernel segments.
   */
  const paddr_t data = RISCV_PHYSADDR(__data);
  const paddr_t ebss = KERNEL_IMG_END;

  vaddr_t va = (vaddr_t)__text;
  paddr_t pa = RISCV_PHYSADDR(va);

  /* Read-only segment - sections: .text and .rodata. */
  for (; pa < data; pa += PAGESIZE, va += PAGESIZE)
    pte[L1_INDEX(va)] = PA_TO_PTE(pa) | PTE_X | PTE_KERN_RO;

  /* Read-write segment - sections: .data and .bss. */
  for (; pa < ebss; pa += PAGESIZE, va += PAGESIZE)
    pte[L1_INDEX(va)] = PA_TO_PTE(pa) | PTE_KERN;

  /*
   * NOTE: we don't have to map the boot allocation area as the allocated
   * data will only be accessed using physical addresses (see pmap).
   */
}

__boot_text static void map_dtb(paddr_t dtb, pd_entry_t *pde) {
  /* Assume that dbt will be covered by single superpage (4MiB). */
  const size_t idx = L0_INDEX(BOOT_DTB_VADDR);
  pde[idx] = PA_TO_PTE(dtb) | PTE_KERN;
}

__boot_text static void map_pd(pd_entry_t *pde) {
  /* For simplicity, we map the page directory using a superpage. */
  const size_t idx = L0_INDEX(BOOT_PD_VADDR);
  pde[idx] = PA_TO_PTE((paddr_t)pde) | PTE_KERN;
}

__boot_text static void vm_page_ensure_pts(pd_entry_t *pde) {
  const size_t idx = L0_INDEX((vaddr_t)__text);

  for (int i = 0; i < VM_PAGE_PDS; i++) {
    pde[idx + i + 1] = PA_TO_PTE((paddr_t)bootmem_alloc(PAGESIZE)) | PTE_V;
  }
}

static __noreturn void riscv_boot(paddr_t dtb, paddr_t pde);

__boot_text __noreturn void riscv_init(paddr_t dtb) {
  if (!(__eboot < __kernel_start || __kernel_end < __boot))
    halt();

  bootmem_init();

  /* Create kernel page directory. */
  pd_entry_t *pde = bootmem_alloc(PAGESIZE);

  map_kernel_image(pde);
  map_dtb(dtb, pde);
  map_pd(pde);
  vm_page_ensure_pts(pde);

  /* Temporarily set the trap vector. */
  csr_write(stvec, riscv_boot);

  /*
   * Move to VM boot stage.
   */
  const paddr_t satp = SATP_MODE_SV32 | ((paddr_t)pde >> PAGE_SHIFT);
  void *boot_sp = &boot_stack[PAGESIZE];

  __sfence_vma();

  __asm __volatile("mv a0, %0\n\t"
                   "mv a1, %1\n\t"
                   "mv sp, %2\n\t"
                   "csrw satp, %3\n\t"
                   "nop" /* triggers instruction fetch page fault */
                   :
                   : "r"(dtb), "r"(pde), "r"(boot_sp), "r"(satp));

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

extern void *board_stack(paddr_t dtb_pa, vaddr_t dtb_va);
extern void __noreturn board_init(void);

static __noreturn void riscv_boot(paddr_t dtb, paddr_t pde) {
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

  /*
   * Initialize first kernel submodules.
   */
  init_kasan();
  init_klog();

  void *sp = board_stack(dtb, BOOT_DTB_VADDR);

  pmap_bootstrap(pde, BOOT_PD_VADDR);

  /*
   * Switch to thread0's stack and perform `board_init`.
   */
  __asm __volatile("mv sp, %0\n\t"
                   "tail board_init" ::"r"(sp));
  __unreachable();
}

/*
 * TODO(MichalBlk): remove those after architecture split of dbg debug
 * scripts.
 */
typedef struct {
} tlbentry_t;

static __unused __boot_data const volatile tlbentry_t _gdb_tlb_entry;
