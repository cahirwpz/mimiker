#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/dtb.h>
#include <sys/fdt.h>
#include <sys/kmem.h>
#include <sys/vm.h>

/*
 * offset of dtb on first page that contains device tree blob
 *
 * NOTE: dtb can be loaded into middle of page so we need to adjust start of vm
 * mapping for that page.
 */
static uintptr_t _dtb_offset = 0;
/* root of dtb in va */
static void *_dtb_root = NULL;
/* phys addr of dtb_root */
static paddr_t _dtb_root_pa = 0;
/* size of dtb */
static size_t _dtb_size = 0;

paddr_t dtb_early_root(void) {
  assert(_dtb_root_pa != 0);
  return _dtb_root_pa + _dtb_offset;
}

void dtb_early_init(paddr_t dtb, size_t size) {
  _dtb_root_pa = rounddown(dtb, PAGESIZE);
  _dtb_offset = dtb - _dtb_root_pa;
  _dtb_size = size + _dtb_offset;
}

void *dtb_root(void) {
  assert(_dtb_root != NULL);
  return _dtb_root + _dtb_offset;
}

void dtb_init(void) {
  size_t dtb_size = roundup(_dtb_size, PAGESIZE);
  if (_dtb_root_pa != 0)
    _dtb_root = (void *)kmem_map_contig(_dtb_root_pa, dtb_size, 0);
}
