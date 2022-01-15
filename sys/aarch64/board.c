#include <sys/kenv.h>
#include <sys/cmdline.h>
#include <sys/libkern.h>
#include <sys/klog.h>
#include <sys/initrd.h>
#include <sys/thread.h>
#include <sys/vm_physmem.h>
#include <sys/context.h>
#include <sys/interrupt.h>
#include <sys/kasan.h>
#include <sys/dtb.h>
#include <aarch64/mcontext.h>
#include <aarch64/vm_param.h>
#include <aarch64/pmap.h>

static void process_dtb(char **tokens, kstack_t *stk) {
  char buf[32];
  uint64_t start, size;

  /*
   * Memory boundaries.
   * TODO: we assume that physical memory starts at fixed address 0.
   * This assumption should be removed and the memory boundaries
   * should be read from dtb (thus `start` shouldn't be discarded).
   */
  dtb_mem(&start, &size);
  snprintf(buf, sizeof(buf), "memsize=%lu", size);
  tokens = cmdline_extract_tokens(stk, buf, tokens);

  /* Initrd boundaries. */
  dtb_rd(&start, &size);
  snprintf(buf, sizeof(buf), "rd_start=%lu", start);
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "rd_size=%lu", size);
  tokens = cmdline_extract_tokens(stk, buf, tokens);

  /* Kernel cmdline. */
  tokens = cmdline_extract_tokens(stk, dtb_cmdline(), tokens);
  *tokens = NULL;
}

void *board_stack(paddr_t dtb) {
  init_kasan();
  init_klog();

  dtb_early_init(dtb, PHYS_TO_DMAP(dtb));

  kstack_t *stk = &thread0.td_kstack;

  thread0.td_uctx = kstack_alloc_s(stk, mcontext_t);

  /*
   * NOTE: memsize, rd_start, rd_size, cmdline + 2 = 6
   */
  char **kenvp = kstack_alloc(stk, 6 * sizeof(char *));
  process_dtb(kenvp, stk);
  kstack_fix_bottom(stk);
  init_kenv(kenvp);

  return stk->stk_ptr;
}

typedef struct {
  paddr_t start;
  paddr_t end;
} addr_range_t;

static int addr_range_cmp(const void *_lhs, const void *_rhs) {
  const addr_range_t *lhs = _lhs;
  const addr_range_t *rhs = _rhs;

  assert(lhs->start != rhs->start);

  if (lhs->start < rhs->start)
    return -1;
  return 1;
}

static void rpi3_physmem(void) {
  /* XXX: workaround - pmap_enter fails to physical page with address 0 */
  paddr_t ram_start = PAGESIZE;
  paddr_t ram_end = kenv_get_ulong("memsize");
  paddr_t kern_start = (paddr_t)__boot;
  paddr_t kern_end = (paddr_t)_bootmem_end;
  paddr_t rd_start = ramdisk_get_start();
  paddr_t rd_end = rd_start + ramdisk_get_size();
  paddr_t dtb_start = rounddown(dtb_early_root(), PAGESIZE);
  paddr_t dtb_end = dtb_start + dtb_size();

  /* TODO(pj) if vm_physseg_plug* interface was more flexible,
   * we could do without following hack, please refer to issue #1129 */
  addr_range_t memory[3] = {
    {.start = rounddown(kern_start, PAGESIZE),
     .end = roundup(kern_end, PAGESIZE)},
    {.start = rounddown(rd_start, PAGESIZE), .end = roundup(rd_end, PAGESIZE)},
    {.start = rounddown(dtb_start, PAGESIZE),
     .end = roundup(dtb_end, PAGESIZE)},
  };

  qsort(memory, 3, sizeof(addr_range_t), addr_range_cmp);

  vm_physseg_plug(ram_start, memory[0].start);
  for (int i = 0; i < 3; ++i) {
    if (memory[i].start == memory[i].end)
      continue;
    assert(memory[i].start < memory[i].end);
    vm_physseg_plug_used(memory[i].start, memory[i].end);
    if (i == 2) {
      if (memory[i].end < ram_end)
        vm_physseg_plug(memory[i].end, ram_end);
    } else if (memory[i].end < memory[i + 1].start) {
      vm_physseg_plug(memory[i].end, memory[i + 1].start);
    }
  }
}

__noreturn void board_init(void) {
  rpi3_physmem();
  intr_enable();
  kernel_init();
}
