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

static void rpi3_physmem(void) {
  /* XXX: workaround - pmap_enter fails to physical page with address 0 */
  paddr_t ram_start = PAGESIZE;
  paddr_t ram_end = kenv_get_ulong("memsize");
  paddr_t kern_start = (paddr_t)__boot;
  paddr_t kern_end = (paddr_t)_bootmem_end;
  paddr_t rd_start = ramdisk_get_start();
  paddr_t rd_end = rd_start + ramdisk_get_size();

  vm_physseg_plug(ram_start, kern_start);
  vm_physseg_plug_used(kern_start, kern_end);

  if (rd_start != rd_end) {
    vm_physseg_plug(kern_end, rd_start);
    vm_physseg_plug_used(rd_start, rd_end);
    vm_physseg_plug(rd_end, ram_end);
  } else {
    vm_physseg_plug(kern_end, ram_end);
  }
}

__noreturn void board_init(void) {
  init_kasan();
  init_klog();
  rpi3_physmem();
  intr_enable();
  kernel_init();
}
