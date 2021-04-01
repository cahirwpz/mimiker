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
#include <aarch64/atags.h>
#include <aarch64/mcontext.h>
#include <aarch64/vm_param.h>

static int count_dtb(void *dtb) {
  /* memsize, rd_start, rd_size, cmdline */
  int ntokens = 4;
  return ntokens;
}

static int process_dtb_memsize(void *dtb) {
  const char *path = "/memory@0";
  const char *name = "reg";
  const int *prop;
  int offset;
  int len;

  offset = fdt_path_offset(dtb, path);
  prop = (const int *)fdt_getprop(dtb, offset, name, &len);
  if (prop == NULL || (size_t)len < 2 * sizeof(int))
    panic("Failed to get memory size from FDT!");

  return fdt32_to_cpu(prop[1]);
}

static int process_dtb_rd_start(void *dtb) {
  const char *path = "/chosen";
  const char *name = "linux,initrd-start";
  const int *prop;
  int offset;
  int len;

  offset = fdt_path_offset(dtb, path);
  prop = (const int *)fdt_getprop(dtb, offset, name, &len);
  if (prop == NULL)
    panic("Failed to get initrd-start from FDT!");

  return fdt32_to_cpu(*prop);
}

static int process_dtb_rd_size(void *dtb) {
  const char *path = "/chosen";
  const char *name = "linux,initrd-end";
  const int *prop;
  int offset;
  int len;

  offset = fdt_path_offset(dtb, path);
  prop = (const int *)fdt_getprop(dtb, offset, name, &len);
  if (prop == NULL)
    panic("Failed to get initrd-end from FDT!");

  return fdt32_to_cpu(*prop) - process_dtb_rd_start(dtb);
}

static const char *process_dtb_cmdline(void *dtb) {
  const char *path = "/chosen";
  const char *name = "bootargs";
  const char *prop;
  int offset;

  offset = fdt_path_offset(dtb, path);
  prop = (const char *)fdt_getprop(dtb, offset, name, NULL);
  if (prop == NULL)
    panic("Failed to get cmdline from FDT!");

  return prop;
}

static void process_dtb(char **tokens, kstack_t *stk, void *dtb) {
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

void *board_stack(void *dtb) {
  dtb = (void *)PHYS_TO_DMAP(dtb);
  if (fdt_check_header(dtb))
    panic("FDT incorrect header!");

  kstack_t *stk = &thread0.td_kstack;

  thread0.td_uctx = kstack_alloc_s(stk, mcontext_t);

  int ntokens = count_dtb(dtb);
  char **kenvp = kstack_alloc(stk, (ntokens + 2) * sizeof(char *));
  process_dtb(kenvp, stk, dtb);
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
