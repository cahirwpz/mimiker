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
#include <sys/fdt.h>
#include <sys/dtb.h>
#include <aarch64/mcontext.h>
#include <aarch64/vm_param.h>
#include <aarch64/pmap.h>

/* Return offset of path at dtb or die. */
static int dtb_offset(void *dtb, const char *path) {
  int offset = fdt_path_offset(dtb, path);
  if (offset < 0)
    panic("Failed to find offset at dtb!");
  return offset;
}

static uint32_t process_dtb_memsize(void *dtb) {
  int offset = dtb_offset(dtb, "/memory@0");
  int len;
  const uint32_t *prop = fdt_getprop(dtb, offset, "reg", &len);
  /* reg contains start (4 bytes) and end (4 bytes) */
  if (prop == NULL || (size_t)len < 2 * sizeof(uint32_t))
    panic("Failed to get memory size from dtb!");

  return fdt32_to_cpu(prop[1]);
}

static int process_dtb_rd_start(void *dtb) {
  int offset = dtb_offset(dtb, "/chosen");
  int len;
  const uint32_t *prop = fdt_getprop(dtb, offset, "linux,initrd-start", &len);
  if (prop == NULL || (size_t)len < sizeof(uint32_t))
    panic("Failed to get initrd-start from dtb!");

  return fdt32_to_cpu(*prop);
}

static int process_dtb_rd_size(void *dtb) {
  int offset = dtb_offset(dtb, "/chosen");
  int len;
  const uint32_t *prop = fdt_getprop(dtb, offset, "linux,initrd-end", &len);
  if (prop == NULL || (size_t)len < sizeof(uint32_t))
    panic("Failed to get initrd-start from dtb!");

  return fdt32_to_cpu(*prop) - process_dtb_rd_start(dtb);
}

static const char *process_dtb_cmdline(void *dtb) {
  int offset = dtb_offset(dtb, "/chosen");
  const char *prop = fdt_getprop(dtb, offset, "bootargs", NULL);
  if (prop == NULL)
    panic("Failed to get cmdline from dtb!");

  return prop;
}

static void process_dtb(char **tokens, kstack_t *stk, void *dtb) {
  if (fdt_check_header(dtb))
    panic("dtb incorrect header!");

  char buf[32];
  snprintf(buf, sizeof(buf), "memsize=%d", process_dtb_memsize(dtb));
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "rd_start=%d", process_dtb_rd_start(dtb));
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  snprintf(buf, sizeof(buf), "rd_size=%d", process_dtb_rd_size(dtb));
  tokens = cmdline_extract_tokens(stk, buf, tokens);
  tokens = cmdline_extract_tokens(stk, process_dtb_cmdline(dtb), tokens);
  *tokens = NULL;
}

void *board_stack(paddr_t dtb) {
  dtb_early_init(dtb, fdt_totalsize(PHYS_TO_DMAP(dtb)));

  kstack_t *stk = &thread0.td_kstack;

  thread0.td_uctx = kstack_alloc_s(stk, mcontext_t);

  /*
   * NOTE: memsize, rd_start, rd_size, cmdline + 2 = 6
   */
  char **kenvp = kstack_alloc(stk, 6 * sizeof(char *));
  process_dtb(kenvp, stk, (void *)PHYS_TO_DMAP(dtb));
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
  paddr_t dtb_start = dtb_early_root();
  paddr_t dtb_end = dtb_start + fdt_totalsize(PHYS_TO_DMAP(dtb_start));

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
  init_kasan();
  klog_config();
  rpi3_physmem();
  intr_enable();
  kernel_init();
}
