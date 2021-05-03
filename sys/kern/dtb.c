#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/dtb.h>
#include <sys/fdt.h>
#include <sys/kmem.h>
#include <sys/vm.h>

/* root of dtb in va */
static void *_dtb_root = NULL;
/* phys addr of dtb_root */
static paddr_t _dtb_root_pa = 0;
/* size of dtb */
static size_t _dtb_size = 0;

paddr_t dtb_early_root(void) {
  assert(_dtb_root_pa != 0);
  return _dtb_root_pa;
}

void dtb_early_init(paddr_t dtb, size_t size) {
  _dtb_root_pa = dtb;
  _dtb_size = size;
}

void *dtb_root(void) {
  assert(_dtb_root != NULL);
  return _dtb_root;
}

void dtb_init(void) {
  size_t dtb_size = roundup(_dtb_size, PAGESIZE);
  if (_dtb_root_pa != 0)
    _dtb_root = (void *)kmem_map_contig(_dtb_root_pa, dtb_size, 0);
}
