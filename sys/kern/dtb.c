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

paddr_t dtb_early_root(void) {
  assert(_dtb_root_pa != 0);
  return _dtb_root_pa + _dtb_offset;
}

void dtb_early_init(paddr_t pa, vaddr_t va) {
  void *dtb = (void *)va;

  if (fdt_check_header(dtb))
    panic("dtb incorrect header!");

  size_t size = fdt_totalsize(dtb);

  _dtb_root = (void *)va;
  _dtb_root_pa = rounddown(pa, PAGESIZE);
  _dtb_offset = pa - _dtb_root_pa;
  _dtb_size = roundup(size + _dtb_offset, PAGESIZE);
}

void *dtb_root(void) {
  assert(_dtb_root != NULL);
  return _dtb_root + _dtb_offset;
}

size_t dtb_size(void) {
  return _dtb_size;
}

void dtb_init(void) {
  if (_dtb_root_pa != 0)
    _dtb_root = (void *)kmem_map_contig(_dtb_root_pa, _dtb_size, 0);
}

static int dtb_offset(const char *path) {
  int offset = fdt_path_offset(_dtb_root, path);
  if (offset < 0)
    panic("Failed to find dtb offset for %s!");
  return offset;
}

static int dtb_addr_cells(int node) {
  int len;
  const uint32_t *prop = fdt_getprop(_dtb_root, node, "#address-cells", &len);
  /* #address-cells is a 4-byte property */
  if (prop == NULL || (size_t)len != sizeof(uint32_t))
    panic("Failed to retreive #address-cells property for node %d!", node);
  return fdt32_to_cpu(*prop);
}

static unsigned long dtb_to_cpu(const uint32_t *prop, int addr_cells) {
  if (addr_cells == 1)
    return fdt32_to_cpu(*prop);
  assert(addr_cells == 2);
  return fdt64_to_cpu(*(unsigned long *)prop);
}

/*
 * XXX: FTTB, we consider only a single pair <addr size>.
 */
void dtb_reg(int parent, int node, unsigned long *addr_p,
             unsigned long *size_p) {
  int len;
  const int addr_cells = dtb_addr_cells(parent);
  const size_t pair_size = 2 * addr_cells * sizeof(uint32_t);
  const uint32_t *prop = fdt_getprop(_dtb_root, node, "reg", &len);
  if (prop == NULL || !is_aligned(len, pair_size))
    panic("Invalid reg property in node %d (parent=%d)!", node, parent);

  *addr_p = dtb_to_cpu(prop, addr_cells);
  *size_p = dtb_to_cpu(prop + addr_cells, addr_cells);
}

unsigned long dtb_cell(int parent, int node, const char *name) {
  int len;
  const int addr_cells = dtb_addr_cells(parent);
  const uint32_t *prop = fdt_getprop(_dtb_root, node, name, &len);
  if (prop == NULL || ((size_t)len != addr_cells * sizeof(uint32_t)))
    panic("Invalid %s property in node %d (parent=%d)!", name, node, parent);
  return dtb_to_cpu(prop, addr_cells);
}

void dtb_mem(unsigned long *addr_p, unsigned long *size_p) {
  int node = dtb_offset("/memory");
  dtb_reg(DTB_ROOT_NODE, node, addr_p, size_p);
}

/*
 * XXX: FTTB, we assume a single reserved memory range.
 */
void dtb_rsvdmem(unsigned long *addr_p, unsigned long *size_p) {
  int node = dtb_offset("reserved-memory");
  int subnode = fdt_first_subnode(_dtb_root, node);
  if (subnode < 0)
    panic("reserved-memory node doesn't contain any range!");
  dtb_reg(node, subnode, addr_p, size_p);
}

void dtb_rd(unsigned long *addr_p, unsigned long *size_p) {
  int node = dtb_offset("/chosen");
  *addr_p = dtb_cell(DTB_ROOT_NODE, node, "linux,initrd-start");
  *size_p = dtb_cell(DTB_ROOT_NODE, node, "linux,initrd-end") - *addr_p;
}

const char *dtb_cmdline(void) {
  int offset = dtb_offset("/chosen");
  const char *prop = fdt_getprop(_dtb_root, offset, "bootargs", NULL);
  if (prop == NULL)
    panic("Invalid bootargs property!");
  return prop;
}
