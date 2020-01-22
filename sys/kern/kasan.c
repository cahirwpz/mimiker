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

#define __MD_VIRTUAL_SHIFT 25
#define __MD_SHADOW_SIZE                                                       \
  (1ULL << (__MD_VIRTUAL_SHIFT - KASAN_SHADOW_SCALE_SHIFT))
#define __MD_CANONICAL_BASE 0xC0000000

#define KASAN_MD_SHADOW_START 0xF0000000
#define KASAN_MD_SHADOW_END (KASAN_MD_SHADOW_START + __MD_SHADOW_SIZE)

static int kasan_ready;

__always_inline static inline int8_t *kasan_md_addr_to_shad(const void *addr) {
  vaddr_t va = (vaddr_t)addr;
  return (int8_t *)(KASAN_MD_SHADOW_START +
                    ((va - __MD_CANONICAL_BASE) >> KASAN_SHADOW_SCALE_SHIFT));
}

#define ADDR_CROSSES_SCALE_BOUNDARY(addr, size)                                \
  (addr >> KASAN_SHADOW_SCALE_SHIFT) !=                                        \
    ((addr + size - 1) >> KASAN_SHADOW_SCALE_SHIFT)

__always_inline static inline bool
kasan_shadow_1byte_isvalid(unsigned long addr, uint8_t *code) {
  int8_t *byte = kasan_md_addr_to_shad((void *)addr);
  int8_t last = (addr & KASAN_SHADOW_MASK) + 1;

  if (__predict_true(*byte == 0 || last <= *byte))
    return true;
  *code = *byte;
  return false;
}

__always_inline static inline bool
kasan_shadow_2byte_isvalid(unsigned long addr, uint8_t *code) {
  if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 2))
    return (kasan_shadow_1byte_isvalid(addr, code) &&
            kasan_shadow_1byte_isvalid(addr + 1, code));

  int8_t *byte = kasan_md_addr_to_shad((void *)addr);
  int8_t last = ((addr + 1) & KASAN_SHADOW_MASK) + 1;

  if (__predict_true(*byte == 0 || last <= *byte))
    return true;
  *code = *byte;
  return false;
}

__always_inline static inline bool
kasan_shadow_4byte_isvalid(unsigned long addr, uint8_t *code) {
  if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 4))
    return (kasan_shadow_2byte_isvalid(addr, code) &&
            kasan_shadow_2byte_isvalid(addr + 2, code));

  int8_t *byte = kasan_md_addr_to_shad((void *)addr);
  int8_t last = ((addr + 3) & KASAN_SHADOW_MASK) + 1;

  if (__predict_true(*byte == 0 || last <= *byte))
    return true;
  *code = *byte;
  return false;
}

__always_inline static inline bool
kasan_shadow_8byte_isvalid(unsigned long addr, uint8_t *code) {
  if (ADDR_CROSSES_SCALE_BOUNDARY(addr, 8))
    return (kasan_shadow_4byte_isvalid(addr, code) &&
            kasan_shadow_4byte_isvalid(addr + 4, code));

  int8_t *byte = kasan_md_addr_to_shad((void *)addr);
  int8_t last = ((addr + 7) & KASAN_SHADOW_MASK) + 1;

  if (__predict_true(*byte == 0 || last <= *byte))
    return true;
  *code = *byte;
  return false;
}

__always_inline static inline bool
kasan_shadow_Nbyte_isvalid(unsigned long addr, size_t size, uint8_t *code) {
  for (size_t i = 0; i < size; i++)
    if (__predict_false(!kasan_shadow_1byte_isvalid(addr + i, code)))
      return false;
  return true;
}

__always_inline static inline bool kasan_md_supported(vaddr_t addr) {
  return addr >= __MD_CANONICAL_BASE &&
         addr < __MD_CANONICAL_BASE + (1 << __MD_VIRTUAL_SHIFT);
}

__always_inline static inline void kasan_shadow_check(unsigned long addr,
                                                      size_t size, bool read) {
  if (__predict_false(!kasan_ready))
    return;
  if (!kasan_md_supported(addr))
    return;

  uint8_t code = 0;
  bool valid = true; // will be overwritten
  if (__builtin_constant_p(size)) {
    switch (size) {
      case 1:
        valid = kasan_shadow_1byte_isvalid(addr, &code);
        break;
      case 2:
        valid = kasan_shadow_2byte_isvalid(addr, &code);
        break;
      case 4:
        valid = kasan_shadow_4byte_isvalid(addr, &code);
        break;
      case 8:
        valid = kasan_shadow_8byte_isvalid(addr, &code);
        break;
    }
  } else {
    valid = kasan_shadow_Nbyte_isvalid(addr, size, &code);
  }

  if (__predict_false(!valid))
    panic("KASAN: invalid access to addr %p (%s, %lu bytes, code %d)",
          (void *)addr, (read ? "read" : "write"), size, code);
}

/*
 * In an area of size 'sz_with_redz', mark the 'size' first bytes as valid,
 * and the rest as invalid. There are generally two use cases:
 *  - kasan_mark(addr, origsize, size, code), with origsize < size. This marks
 *    the redzone at the end of the buffer as invalid.
 *  - kasan_mark(addr, size, size, 0). This marks the entire buffer as valid.
 */
void kasan_mark(const void *addr, size_t size, size_t sz_with_redz,
                uint8_t code) {
  size_t i, n, redz;
  int8_t *shad;

  assert((vaddr_t)addr % KASAN_SHADOW_SCALE_SIZE == 0);
  redz = sz_with_redz - roundup(size, KASAN_SHADOW_SCALE_SIZE);
  assert(redz % KASAN_SHADOW_SCALE_SIZE == 0);
  shad = kasan_md_addr_to_shad(addr);

  /* Chunks of 8 bytes, valid. */
  n = size / KASAN_SHADOW_SCALE_SIZE;
  for (i = 0; i < n; i++) {
    *shad++ = 0;
  }

  /* Possibly one chunk, mid. */
  if ((size & KASAN_SHADOW_MASK) != 0) {
    *shad++ = (size & KASAN_SHADOW_MASK);
  }

  /* Chunks of 8 bytes, invalid. */
  n = redz / KASAN_SHADOW_SCALE_SIZE;
  for (i = 0; i < n; i++) {
    *shad++ = code;
  }
}

static void kasan_ctors(void) {
  extern uint32_t __CTOR_LIST__, __CTOR_END__;
  size_t nentries, i;
  uint32_t *ptr;

  nentries =
    ((size_t)&__CTOR_END__ - (size_t)&__CTOR_LIST__) / sizeof(uintptr_t);

  ptr = &__CTOR_LIST__;
  for (i = 0; i < nentries; i++) {
    void (*func)(void);

    func = (void *)(*ptr);
    (*func)();

    ptr++;
  }
}

void kasan_init(void) {
  uint32_t *ptr = (uint32_t *)KASAN_MD_SHADOW_START;
  uint32_t *end = (uint32_t *)KASAN_MD_SHADOW_END;
  while (ptr < end)
    *ptr++ = 0;

  kasan_ready = 1;
  kasan_ctors();
}

#define DEFINE_ASAN_LOAD_STORE(size)                                           \
  void __asan_load##size##_noabort(unsigned long addr) {                       \
    kasan_shadow_check(addr, size, true);                                      \
  }                                                                            \
  void __asan_store##size##_noabort(unsigned long addr) {                      \
    kasan_shadow_check(addr, size, false);                                     \
  }

DEFINE_ASAN_LOAD_STORE(1);
DEFINE_ASAN_LOAD_STORE(2);
DEFINE_ASAN_LOAD_STORE(4);
DEFINE_ASAN_LOAD_STORE(8);

void __asan_loadN_noabort(unsigned long addr, size_t size) {
  kasan_shadow_check(addr, size, true);
}

void __asan_storeN_noabort(unsigned long addr, size_t size) {
  kasan_shadow_check(addr, size, false);
}

void __asan_handle_no_return(void) {
}

struct __asan_global_source_location {
  const char *filename;
  int line_no;
  int column_no;
};

struct __asan_global {
  const void *beg;          /* address of the global variable */
  size_t size;              /* size of the global variable */
  size_t size_with_redzone; /* size with the redzone */
  const void *name;         /* name of the variable */
  const void *module_name;  /* name of the module where the var is declared */
  unsigned long has_dynamic_init; /* the var has dyn initializer (c++) */
  struct __asan_global_source_location *location;
  uintptr_t odr_indicator; /* the address of the ODR indicator symbol */
};

void __asan_register_globals(struct __asan_global *globals, size_t n) {
  for (size_t i = 0; i < n; i++)
    kasan_mark(globals[i].beg, globals[i].size, globals[i].size_with_redzone,
               0xFF);
}

void __asan_unregister_globals(struct __asan_global *globals, size_t n) {
}
