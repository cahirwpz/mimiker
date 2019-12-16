#include <sys/types.h>
#include <sys/mimiker.h>
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/vm.h>
#include <sys/types.h>
#include <sys/vm_physmem.h>
#include <sys/pmap.h>
#include <machine/vm_param.h>
#include <sys/param.h>

#define KASAN_SHADOW_SCALE_SHIFT 3
#define KASAN_SHADOW_SCALE_SIZE (1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK (KASAN_SHADOW_SCALE_SIZE - 1)

#define __MD_VIRTUAL_SHIFT 30
#define __MD_SHADOW_SIZE                                                       \
  (1ULL << (__MD_VIRTUAL_SHIFT - KASAN_SHADOW_SCALE_SHIFT))
#define __MD_CANONICAL_BASE 0xC0000000

#define KASAN_MD_SHADOW_START 0xF0000000
#define KASAN_MD_SHADOW_END (KASAN_MD_SHADOW_START + __MD_SHADOW_SIZE)

static int kasan_ready;
static int kasan_working;

static int8_t *kasan_md_addr_to_shad(const void *addr) {
  vaddr_t va = (vaddr_t)addr;
  return (int8_t *)(KASAN_MD_SHADOW_START +
                    ((va - __MD_CANONICAL_BASE) >> KASAN_SHADOW_SCALE_SHIFT));
}

static bool kasan_shadow_1byte_isvalid(unsigned long addr) {
  int8_t *byte = kasan_md_addr_to_shad((void *)addr);
  int8_t last = (addr & KASAN_SHADOW_MASK) + 1;
  return *byte == 0 || last <= *byte;
}

static bool kasan_shadow_Nbyte_isvalid(unsigned long addr, size_t size) {
  for (size_t i = 0; i < size; i++)
    if (!kasan_shadow_1byte_isvalid(addr + i))
      return false;
  return true;
}

static bool kasan_md_supported(vaddr_t addr) {
  return addr >= 0xC0000000 && addr <= 0xFFFFFFFF;
}

static void kasan_shadow_check(unsigned long addr, size_t size) {
  if (!kasan_ready || kasan_working || !kasan_md_supported(addr))
    return;

  if (!kasan_shadow_Nbyte_isvalid(addr, size))
    panic_fail();

  kasan_working = 1;
  // klog("kasan_shadow_check(addr=%p, size=%d)", addr, size);
  kasan_working = 0;
}

static void kasan_md_shadow_map_page(vaddr_t va) {
  vm_page_t *pg = vm_page_alloc(1);
  if (pg == NULL)
    panic_fail();
  paddr_t pa = pg->paddr;
  pmap_kenter(va, pa, VM_PROT_READ | VM_PROT_WRITE);
}

void kasan_shadow_map(void *addr, size_t size) {
  assert((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);
  size = roundup(size, KASAN_SHADOW_SCALE_SIZE) / KASAN_SHADOW_SCALE_SIZE;

  vaddr_t sva = (vaddr_t)kasan_md_addr_to_shad(addr);
  vaddr_t eva = (vaddr_t)kasan_md_addr_to_shad(addr) + size;

  sva = rounddown(sva, PAGESIZE);
  eva = roundup(eva, PAGESIZE);

  int npages = (eva - sva) / PAGESIZE;

  assert(sva >= KASAN_MD_SHADOW_START && eva < KASAN_MD_SHADOW_END);

  for (int i = 0; i < npages; i++)
    kasan_md_shadow_map_page(sva + i * PAGESIZE);
}

void kasan_init(void) {
  kasan_shadow_map(__kernel_start,
                   (vaddr_t)vm_kernel_end - (vaddr_t)__kernel_start);
  kasan_ready = 1;
}

#define DEFINE_ASAN_LOAD_STORE(size)                                           \
  void __asan_load##size(unsigned long addr) {                                 \
    kasan_shadow_check(addr, size);                                            \
  }                                                                            \
  void __asan_load##size##_noabort(unsigned long addr) {                       \
    kasan_shadow_check(addr, size);                                            \
  }                                                                            \
  void __asan_store##size(unsigned long addr) {                                \
    kasan_shadow_check(addr, size);                                            \
  }                                                                            \
  void __asan_store##size##_noabort(unsigned long addr) {                      \
    kasan_shadow_check(addr, size);                                            \
  }

DEFINE_ASAN_LOAD_STORE(1);
DEFINE_ASAN_LOAD_STORE(2);
DEFINE_ASAN_LOAD_STORE(4);
DEFINE_ASAN_LOAD_STORE(8);
DEFINE_ASAN_LOAD_STORE(16);

void __asan_loadN(unsigned long addr, size_t size) {
  kasan_shadow_check(addr, size);
}

void __asan_loadN_noabort(unsigned long addr, size_t size) {
  kasan_shadow_check(addr, size);
}

void __asan_storeN(unsigned long addr, size_t size) {
  kasan_shadow_check(addr, size);
}

void __asan_storeN_noabort(unsigned long addr, size_t size) {
  kasan_shadow_check(addr, size);
}

void __asan_handle_no_return(void) {
}
