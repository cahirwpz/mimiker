#include <stddef.h>
#include <stdint.h>
#include <sys/cmdline.h>
#include <sys/dtb.h>
#include <sys/initrd.h>
#include <sys/interrupt.h>
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/vm_physmem.h>
#include <riscv/mcontext.h>
#include <riscv/vm_param.h>

static size_t count_args(void) {
  /*
   * Tokens:
   *   - mem_start, mem_size,
   *   - rsvdmem_start, rsvdmem_size,
   *   - rd_start, rd_size,
   *   - tokens in cmdline.
   */
  size_t ntokens = 6;
  const char *cmdline = dtb_cmdline();
  ntokens += cmdline_count_tokens(cmdline);
  return ntokens;
}

static void process_dtb(char **tokens, kstack_t *stk) {
  char buf[32];
  uint64_t start, size;

  /* Memory boundaries. */
  dtb_mem(&start, &size);
  snprintf(buf, sizeof(buf), "mem_start=%llu", start);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "mem_size=%llu", size);
  tokens = cmdline_extract_tokens(stk, buf, tokens);

  /* Reserved memory boundaries. */
  dtb_rsvdmem(&start, &size);
  snprintf(buf, sizeof(buf), "rsvdmem_start=%llu", start);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "rsvdmem_size=%llu", size);
  tokens = cmdline_extract_tokens(stk, buf, tokens);

  /* Initrd memory boundaries. */
  dtb_rd(&start, &size);
  snprintf(buf, sizeof(buf), "rd_start=%llu", start);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "rd_size=%llu", size);
  tokens = cmdline_extract_tokens(stk, buf, tokens);

  /* Kernel cmdline. */
  tokens = cmdline_extract_tokens(stk, dtb_cmdline(), tokens);
  *tokens = NULL;
}

void *board_stack(paddr_t dtb_pa, vaddr_t dtb_va) {
  dtb_early_init(dtb_pa, dtb_va);

  kstack_t *stk = &thread0.td_kstack;

  /*
   * NOTE: when forking the init process, we will copy thread0's user context
   * to init's user context, therefore `td_uctx` must point to valid memory.
   */
  thread0.td_uctx = kstack_alloc_s(stk, mcontext_t);

  /*
   * NOTE: beside `count_args()` pointers for tokens,
   * we need two additional pointers:
   *   - one for init program name (see `_kinit`),
   *   - one for NULL terminating init program argument vector
   *     (kernel environment is terminated by `cmdline_extract_tokens()`).
   */
  const size_t nptrs = count_args() + 2;
  char **kenvp = kstack_alloc(stk, nptrs * sizeof(char *));

  process_dtb(kenvp, stk);
  kstack_fix_bottom(stk);

  init_kenv(kenvp);

  /* If klog-mask argument has been supplied, let's update the mask. */
  const char *klog_mask = kenv_get("klog-mask");
  if (klog_mask) {
    unsigned mask = strtol(klog_mask, NULL, 16);
    klog_setmask(mask);
  }

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
  paddr_t rsvdmem_start = kenv_get_ulong("rsvdmem_start");
  paddr_t rsvdmem_end = rsvdmem_start + kenv_get_ulong("rsvdmem_size");
  paddr_t kern_start = KERNEL_PHYS;
  paddr_t kern_end = KERNEL_PHYS_END;
  paddr_t rd_start = ramdisk_get_start();
  paddr_t rd_end = rd_start + ramdisk_get_size();
  paddr_t dtb_start = rounddown(dtb_early_root(), PAGESIZE);
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
      .start = START(rsvdmem_start),
      .end = END(rsvdmem_end),
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

  const size_t nranges = sizeof(memory) / sizeof(addr_range_t);
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
  /* TODO(MichalBlk): initialize SBI. */
  physmem_regions();
  intr_enable();
  kernel_init();
}
