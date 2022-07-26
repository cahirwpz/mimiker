#define KL_LOG KL_INIT
#include <sys/boot.h>
#include <sys/cmdline.h>
#include <sys/fdt.h>
#include <sys/initrd.h>
#include <sys/interrupt.h>
#include <sys/kasan.h>
#include <sys/kenv.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/types.h>
#include <sys/vm_physmem.h>
#include <riscv/boot.h>
#include <riscv/mcontext.h>
#include <riscv/pmap.h>
#include <riscv/sbi.h>
#include <riscv/vm_param.h>

static size_t count_args(void) {
  /*
   * Tokens:
   *   - mem_start, mem_end,
   *   - rd_start, rd_size,
   *   - tokens in cmdline.
   */
  size_t ntokens = 4;
  const char *cmdline;
  if (FDT_get_chosen_bootargs(&cmdline))
    panic("Failed to retrieve bootargs from DTB!");
  ntokens += cmdline_count_tokens(cmdline);
  return ntokens;
}

static char **process_dtb_mem(char *buf, size_t buflen, char **tokens,
                              kstack_t *stk) {
  fdt_mem_reg_t mrs[FDT_MAX_REG_TUPLES];
  size_t cnt, size;
  if (FDT_get_mem(mrs, &cnt, &size))
    panic("Failed to retrieve memory regions from DTB!");
  assert(cnt == 1);
  snprintf(buf, buflen, "mem_start=%lu", mrs[0].addr);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, buflen, "mem_end=%lu", mrs[0].addr + mrs[0].size);
  return cmdline_extract_tokens(stk, buf, tokens);
}

static char **process_dtb_initrd(char *buf, size_t buflen, char **tokens,
                                 kstack_t *stk) {
  fdt_mem_reg_t mr;
  if (FDT_get_chosen_initrd(&mr))
    panic("Failed to retrieve initrd boundaries from DTB!");
  snprintf(buf, buflen, "rd_start=%lu", mr.addr);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, buflen, "rd_size=%lu", mr.size);
  return cmdline_extract_tokens(stk, buf, tokens);
}

static char **process_dtb_bootargs(char **tokens, kstack_t *stk) {
  const char *bootargs;
  if (FDT_get_chosen_bootargs(&bootargs))
    panic("Failed to retrieve bootargs from DTB!");
  return cmdline_extract_tokens(stk, bootargs, tokens);
}

static void process_dtb(char **tokens, kstack_t *stk) {
  char buf[32];

  tokens = process_dtb_mem(buf, sizeof(buf), tokens, stk);
  tokens = process_dtb_initrd(buf, sizeof(buf), tokens, stk);
  tokens = process_dtb_bootargs(tokens, stk);

  *tokens = NULL;
}

void *board_stack(void) {
  kstack_t *stk = &thread0.td_kstack;

  /*
   * NOTE: when forking the init process, we will copy thread0's user context
   * to init's user context, therefore `td_uctx` must point to valid memory.
   */
  thread0.td_uctx = kstack_alloc_s(stk, mcontext_t);

  /*
   * NOTE: beside `count_args()` pointers for tokens,
   * we need two additional pointers:
   *   - one to terminate the kernel environment vector,
   *   - one to terminated the argument vector for the init program.
   */
  const size_t nptrs = count_args() + 2;
  char **kenvp = kstack_alloc(stk, nptrs * sizeof(char *));

  process_dtb(kenvp, stk);
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

static void ar_get_kernel_img(addr_range_t *ar) {
  assert(boot_sbrk_end);
  ar->start = PHYSADDR(__kernel_start);
  ar->end = boot_sbrk_end;
}

static void ar_get_initrd(addr_range_t *ar) {
  paddr_t rd_start = kenv_get_ulong("rd_start");
  paddr_t rd_end = rd_start + kenv_get_ulong("rd_size");
  assert(rd_start && rd_end);
  ar->start = START(rd_start);
  ar->end = END(rd_end);
}

static size_t ar_get_reserved_mem(addr_range_t *ars) {
  fdt_mem_reg_t mrs[FDT_MAX_RSV_MEM_REGS];
  size_t cnt;
  if (FDT_get_reserved_mem(mrs, &cnt))
    panic("Failed to retrieve reserved memory regions from DTB!");
  for (size_t i = 0; i < cnt; i++) {
    ars[i].start = START(mrs[i].addr);
    ars[i].end = END(mrs[i].addr + mrs[i].size);
  }
  return cnt;
}

static int ar_cmp(const void *_lhs, const void *_rhs) {
  const addr_range_t *lhs = _lhs;
  const addr_range_t *rhs = _rhs;
  int res = -1;

  if (lhs->start > rhs->start) {
    swap(lhs, rhs);
    res = 1;
  }

  assert(lhs->end <= rhs->start);
  return res;
}

/*
 * Physical memory regions:
 *  - kernel image,
 *  - initial ramdisk,
 *  - DTB blob,
 *  - reserved memory regions (up to `FDT_MAX_RSV_MEM_REGS`).
 */
#define MAX_PHYS_MEM_REGS (FDT_MAX_RSV_MEM_REGS + 3)

static void physmem_regions(void) {
  paddr_t mem_start = kenv_get_ulong("mem_start");
  paddr_t mem_end = kenv_get_ulong("mem_end");
  assert(is_aligned(mem_start, PAGESIZE));
  assert(is_aligned(mem_end, PAGESIZE));

  /*
   * NOTE: please refer to issue #1129 to see why the following workaround
   * is needed.
   */
  addr_range_t memory[MAX_PHYS_MEM_REGS];
  ar_get_kernel_img(&memory[0]);
  ar_get_initrd(&memory[1]);
  const size_t rsvmem_cnt = ar_get_reserved_mem(&memory[2]);

  const size_t nranges = rsvmem_cnt + 2;
  qsort(memory, nranges, sizeof(addr_range_t), ar_cmp);

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
  init_kasan();
  init_klog();
  init_sbi();
  physmem_regions();
  intr_enable();
  kernel_init();
}
