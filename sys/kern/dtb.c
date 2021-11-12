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
/* rounded size of dtb */
static size_t _dtb_size = 0;
/* exact size of dtb */
static size_t _dtb_exact_size = 0;

paddr_t dtb_early_root(void) {
  assert(_dtb_root_pa != 0);
  return _dtb_root_pa + _dtb_offset;
}

void dtb_early_init(paddr_t dtb, size_t size) {
  _dtb_root_pa = rounddown(dtb, PAGESIZE);
  _dtb_offset = dtb - _dtb_root_pa;
  _dtb_size = size + _dtb_offset;
  _dtb_exact_size = size;
}

void *dtb_root(void) {
  assert(_dtb_root != NULL);
  return _dtb_root + _dtb_offset;
}

size_t dtb_size(void) {
  return _dtb_exact_size;
}

void dtb_init(void) {
  size_t dtb_size = roundup(_dtb_size, PAGESIZE);
  if (_dtb_root_pa != 0)
    _dtb_root = (void *)kmem_map_contig(_dtb_root_pa, dtb_size, 0);
}

/*
 * NOTE: the following functions will be called during bootstrap
 * before initializing the klog module.
 */
static void __noreturn halt(void) {
  for (;;)
    ;
}

/* Return offset of path at dtb or die. */
static int dtb_offset(void *dtb, const char *path) {
  int offset = fdt_path_offset(dtb, path);
  if (offset < 0)
    halt();
  return offset;
}

void dtb_mem(void *dtb, uint32_t *start_p, uint32_t *size_p) {
  int offset = dtb_offset(dtb, "/memory");
  int len;
  const uint32_t *prop = fdt_getprop(dtb, offset, "reg", &len);
  /* reg contains start (4 bytes) and size (4 bytes) */
  if (prop == NULL || (size_t)len < 2 * sizeof(uint32_t))
    halt();

  *start_p = fdt32_to_cpu(prop[0]);
  *size_p = fdt32_to_cpu(prop[1]);
}

/*
 * NOTE: we assume there is a single reserved memory range.
 */
void dtb_memrsvd(void *dtb, uint32_t *start_p, uint32_t *size_p) {
  int node_off = dtb_offset(dtb, "reserved-memory");
  int subnode_off = fdt_first_subnode(dtb, node_off);
  if (subnode_off < 0)
    halt();

  int len;
  const uint32_t *prop = fdt_getprop(dtb, subnode_off, "reg", &len);
  /* reg contains start (4 bytes) and size (4 bytes) */
  if (prop == NULL || (size_t)len < 2 * sizeof(uint32_t))
    halt();

  *start_p = fdt32_to_cpu(prop[0]);
  *size_p = fdt32_to_cpu(prop[1]);
}

void dtb_rd(void *dtb, uint32_t *start_p, uint32_t *size_p) {
  int offset = dtb_offset(dtb, "/chosen");
  int len;

  const uint32_t *prop = fdt_getprop(dtb, offset, "linux,initrd-start", &len);
  if (prop == NULL || (size_t)len < sizeof(uint32_t))
    halt();
  *start_p = fdt32_to_cpu(*prop);

  prop = fdt_getprop(dtb, offset, "linux,initrd-end", &len);
  if (prop == NULL || (size_t)len < sizeof(uint32_t))
    halt();
  *size_p = fdt32_to_cpu(*prop) - *start_p;
}

const char *dtb_cmdline(void *dtb) {
  int offset = dtb_offset(dtb, "/chosen");
  const char *prop = fdt_getprop(dtb, offset, "bootargs", NULL);
  if (prop == NULL)
    halt();

  return prop;
}
