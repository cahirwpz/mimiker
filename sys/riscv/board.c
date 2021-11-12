#include <stddef.h>
#include <stdint.h>
#include <sys/cmdline.h>
#include <sys/dtb.h>
#include <sys/fdt.h>
#include <sys/initrd.h>
#include <sys/interrupt.h>
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/vm_physmem.h>
#include <riscv/mcontext.h>
#include <riscv/vm_param.h>

#define __wfi() __asm__ volatile("wfi")

static void __noreturn halt(void) {
  for (;;) {
    __wfi();
  }
}

static size_t count_args(void *dtb) {
  /*
   * NOTE: tokens: mem_start, mem_size, memrsvd_start, memrsvd_size,
   * rd_start, rd_size, tokens in cmdline.
   */
  size_t ntokens = 8;
  const char *cmdline = dtb_cmdline(dtb);
  ntokens += cmdline_count_tokens(cmdline);
  return ntokens;
}

static void process_args(char **tokens, kstack_t *stk, void *dtb) {
  char buf[32];
  uint32_t start, size;

  /* Memory boundaries. */
  dtb_mem(dtb, &start, &size);
  snprintf(buf, sizeof(buf), "mem_start=%u", start);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "mem_size=%u", size);
  tokens = cmdline_extract_tokens(stk, buf, tokens);

  /* Reserved memory boundaries. */
  dtb_memrsvd(dtb, &start, &size);
  snprintf(buf, sizeof(buf), "memrsvd_start=%u", start);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "memrsvd_size=%u", size);
  tokens = cmdline_extract_tokens(stk, buf, tokens);

  /* Initrd memory boundaries. */
  dtb_rd(dtb, &start, &size);
  snprintf(buf, sizeof(buf), "rd_start=%u", start);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "rd_size=%u", size);
  tokens = cmdline_extract_tokens(stk, buf, tokens);

  /* Kernel cmdline. */
  tokens = cmdline_extract_tokens(stk, dtb_cmdline(dtb), tokens);
  *tokens = NULL;
}

void *board_stack(paddr_t dtb_pa, void *dtb_va) {
  if (fdt_check_header(dtb_va))
    halt();

  dtb_early_init(dtb_pa, fdt_totalsize(dtb_va));

  kstack_t *stk = &thread0.td_kstack;

  /*
   * NOTE: when forking the init process, we will copy thread0's user context
   * to init's user context, therefore `td_uctx` must point to a valid memory
   * buffer. Let's just allocate the context on thread0's kernel stack.
   */
  thread0.td_uctx = kstack_alloc_s(stk, mcontext_t);

  size_t ntokens = count_args(dtb_va);
  char **kenvp = kstack_alloc(stk, (ntokens + 2) * sizeof(char *));
  process_args(kenvp, stk, dtb_va);
  kstack_fix_bottom(stk);

  init_kenv(kenvp);

  return stk->stk_ptr;
}

typedef struct {
  paddr_t start;
  paddr_t end;
} addr_range_t;

#define START(pa) rounddown((pa), PAGESIZE)
#define END(pa) roundup((pa), PAGESIZE)

static int addr_range_cmp(const void *_lhs, const void *_rhs) {
  const addr_range_t *lhs = _lhs;
  const addr_range_t *rhs = _rhs;
  int res = -1;

  if (lhs->start > rhs->start) {
    swap(lhs, rhs);
    res = 1;
  }

  assert(lhs->end < rhs->start);
  return res;
}

static void physmem_regions(void) {
  paddr_t mem_start = kenv_get_ulong("mem_start");
  paddr_t mem_end = mem_start + kenv_get_ulong("mem_size");
  paddr_t memrsvd_start = kenv_get_ulong("memrsvd_start");
  paddr_t memrsvd_end = memrsvd_start + kenv_get_ulong("memrsvd_size");
  paddr_t kern_start = (paddr_t)__boot;
  paddr_t kern_end = KERNEL_PHYS_END;
  paddr_t rd_start = ramdisk_get_start();
  paddr_t rd_end = rd_start + ramdisk_get_size();
  paddr_t dtb_start = dtb_early_root();
  paddr_t dtb_end = dtb_start + dtb_size();

  assert(is_aligned(mem_start, PAGESIZE));
  assert(is_aligned(mem_end, PAGESIZE));

  /*
   * NOTE: please refer to issue #1129 to see why the following workaround
   * is needed.
   */
  addr_range_t memory[] = {
    /* reserved memory */
    {
      .start = START(memrsvd_start),
      .end = END(memrsvd_end),
    },
    /* kernel image */
    {
      .start = START(kern_start),
      .end = END(kern_end),
    },
    /* initrd */
    {
      .start = START(rd_start),
      .end = END(rd_end),
    },
    /* dtb */
    {
      .start = START(dtb_start),
      .end = END(dtb_end),
    },
  };

  size_t nranges = sizeof(memory) / sizeof(addr_range_t);
  qsort(memory, nranges, sizeof(addr_range_t), addr_range_cmp);

  addr_range_t *range = &memory[0];

  if (mem_start < range->start)
    vm_physseg_plug(mem_start, range->start);

  for (size_t i = 0; i < nranges - 1; i++, range++) {
    assert(range->start < range->end);
    vm_physseg_plug_used(range->start, range->end);
    vm_physseg_plug(range->end, (range + 1)->start);
  }
  assert(range->start < range->end);
  vm_physseg_plug_used(range->start, range->end);

  if (range->end < mem_end)
    vm_physseg_plug(range->end, mem_end);
}

void __noreturn board_init(void) {
  physmem_regions();
  intr_enable();
  kernel_init();
}
