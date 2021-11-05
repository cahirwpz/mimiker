#include <sys/fdt.h>
#include <sys/mimiker.h>
#include <sys/pcpu.h>
#include <riscv/abi.h>
#include <riscv/pmap.h>
#include <riscv/riscvreg.h>
#include <riscv/vm_param.h>

/*
 * NOTE: Due to Sv32 virtual memory system definition, we have to create a
 * direct mapping for the .boot section. Since we want to be able to use the
 * contents of this section further in the kernel (i.e. `_bootmem_end`)
 * it can't be mapped to user space virtual addresses. Therefore, we assume
 * the kernel image is loaded at a pyhsical address >= start of kernel VM.
 */
static_assert(KERNEL_PHYS >= KERNEL_SPACE_BEGIN,
              "KERNEL_PHYS < KERNEL_SPACE_BEGIN");

#define __wfi() __asm__ volatile("wfi")
#define __set_tp() __asm__ volatile("mv tp, %0" ::"r"(_pcpu_data))
#define __sfence_vma() __asm__ volatile("sfence.vma" ::: "memory")

/* Last physical address used by kernel for boot memory allocation. */
__boot_data void *_bootmem_end;

/* Trap handler in direct mode. */
void cpu_exception_handler(void);

__boot_text static void halt(void) {
  for (;;) {
    __wfi();
  }
}

/* Allocate and clear pages in physical memory just after kernel image end.
 * The argument will be aligned to `PAGESIZE`. */
__boot_text static void *bootmem_alloc(size_t bytes) {
  long *addr = _bootmem_end;
  for (size_t i = 0; i < bytes / sizeof(long); i++)
    addr[i] = 0;
  _bootmem_end += align(bytes, PAGESIZE);
  return addr;
}

__boot_text static void clear_bss(void) {
  long *ptr = (long *)__bss;
  long *end = (long *)__ebss;
  while (ptr < end)
    *ptr++ = 0;
}

__boot_text static void map_kernel_image(pd_entry_t *pde) {
  /* Assume that kernel image will be covered by single PDE (4MiB). */
  pt_entry_t *pte = bootmem_alloc(PAGESIZE);

  /* Set appropriate page directory entry. */
  size_t idx = L0_INDEX((vaddr_t)__boot);
  pde[idx] = PA_TO_PTE((paddr_t)pte) | PTE_KERN;

  /*
   * Map successive kernel segments.
   */
  paddr_t pa = (paddr_t)__boot;
  paddr_t end = (paddr_t)align((paddr_t)__ebss, PAGESIZE);

  const pt_entry_t pte_default = PTE_R | PTE_KERN;

  /* Boot segment - section .boot. */
  for (; pa < (paddr_t)__text; pa += PAGESIZE)
    pte[L1_INDEX(pa)] = PA_TO_PTE(pa) | PTE_W | PTE_X | pte_default;

  /* Read-only segment - sections: .text and .rodata. */
  for (; pa < (paddr_t)__data; pa += PAGESIZE)
    pte[L1_INDEX(pa)] = PA_TO_PTE(pa) | pte_default;

  /* Read-write segment - sections: .data and .bss. */
  for (; pa < end; pa += PAGESIZE)
    pte[L1_INDEX(pa)] = PA_TO_PTE(pa) | PTE_W | pte_default;

  /* NOTE: we don't have to map the boot allocation area as the allocated
   * data will only be accessed using physical addresses. */
}

/* Detect physical memory range. */
__boot_text __unused static void detect_mem(void *dtb, paddr_t *min_pa_p,
                                            size_t *pm_size_p) {
  int nodeoffset = fdt_path_offset(dtb, "/memory");
  if (nodeoffset < 0)
    halt();

  int len;
  const unsigned long *prop = fdt_getprop(dtb, nodeoffset, "reg", &len);
  /* reg = <start size> */
  if (prop == NULL || (size_t)len < 2 * sizeof(unsigned long))
    halt();

  *min_pa_p = fdt32_to_cpu(prop[0]);
  *pm_size_p = fdt32_to_cpu(prop[1]);
}

__boot_text void riscv_init(paddr_t dtb) {
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

  /* Set end address of kernel for boot allocation purposes. */
  _bootmem_end = (void *)align((paddr_t)__ebss, PAGESIZE);

  /* Create kernel page directory. */
  pd_entry_t *pde = bootmem_alloc(PAGESIZE);

  map_kernel_image(pde);

  /*
   * Obtain physical memory boundaries.
   */
  paddr_t min_pa;
  size_t pm_size;
  detect_mem((void *)dtb, &min_pa, &pm_size);

  if (pmap_bootstrap(pde, min_pa, pm_size))
    halt();

  /*
   * Enable MMU:
   *  - MODE = Sv32 - Page-based 32-bit virtual addressing
   */
  __sfence_vma();
  csr_write(satp, ((paddr_t)pde >> PAGE_SHIFT) | SATP_MODE_SV32);
}

/* TODO(MichalBlk): remove those after architecture split of dbg debug
 * scripts. */
typedef struct {
} tlbentry_t;

static __unused __boot_data const volatile tlbentry_t _gdb_tlb_entry;
