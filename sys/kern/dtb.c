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

void dtb_early_init(paddr_t pa, vaddr_t va, size_t size) {
  _dtb_root = (void *)va;
  _dtb_root_pa = rounddown(pa, PAGESIZE);
  _dtb_offset = pa - _dtb_root_pa;
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
int dtb_offset(const char *path) {
  int offset = fdt_path_offset(_dtb_root, path);
  if (offset < 0)
    halt();
  return offset;
}

void dtb_reg(int node, const uint32_t **prop_p, int *len_p) {
  int len;
  const uint32_t *prop = fdt_getprop(_dtb_root, node, "reg", &len);
  /* reg contains start (4 bytes) and size (4 bytes) */
  if (prop == NULL || (size_t)len < 2 * sizeof(uint32_t))
    halt();

  if (prop_p)
    *prop_p = prop;
  if (len_p)
    *len_p = len;
}

void dtb_intr(int node, const uint32_t **prop_p, int *len_p) {
  int len;
  const uint32_t *prop = fdt_getprop(_dtb_root, node, "interrupts", &len);
  if (!prop || (size_t)len < sizeof(uint32_t))
    halt();

  if (prop_p)
    *prop_p = prop;
  if (len_p)
    *len_p = len;
}

void dtb_mem(uint32_t *start_p, uint32_t *size_p) {
  int offset = dtb_offset("/memory");
  const uint32_t *prop;
  dtb_reg(offset, &prop, NULL);
  *start_p = fdt32_to_cpu(prop[0]);
  *size_p = fdt32_to_cpu(prop[1]);
}

/*
 * NOTE: we assume there is a single reserved memory range.
 */
void dtb_memrsvd(uint32_t *start_p, uint32_t *size_p) {
  int node_off = dtb_offset("reserved-memory");
  int subnode_off = fdt_first_subnode(_dtb_root, node_off);
  if (subnode_off < 0)
    halt();
  const uint32_t *prop;
  dtb_reg(subnode_off, &prop, NULL);
  *start_p = fdt32_to_cpu(prop[0]);
  *size_p = fdt32_to_cpu(prop[1]);
}

void dtb_rd(uint32_t *start_p, uint32_t *size_p) {
  int offset = dtb_offset("/chosen");
  int len;

  const uint32_t *prop =
    fdt_getprop(_dtb_root, offset, "linux,initrd-start", &len);
  if (prop == NULL || (size_t)len < sizeof(uint32_t))
    halt();
  *start_p = fdt32_to_cpu(*prop);

  prop = fdt_getprop(_dtb_root, offset, "linux,initrd-end", &len);
  if (prop == NULL || (size_t)len < sizeof(uint32_t))
    halt();
  *size_p = fdt32_to_cpu(*prop) - *start_p;
}

const char *dtb_cmdline(void) {
  int offset = dtb_offset("/chosen");
  const char *prop = fdt_getprop(_dtb_root, offset, "bootargs", NULL);
  if (prop == NULL)
    halt();

  return prop;
}
