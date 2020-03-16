#include <sys/vm.h>
#include <sys/pmap.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/kasan.h>
#include <sys/mimiker.h>
#include <sys/vm_physmem.h>
#include <machine/vm_param.h>

#ifndef KASAN
/* The following functions are defined as no-op inside sys/kasan.h. This would
 * cause a compilation error in this file */
#undef kasan_init
#undef kasan_mark
#endif /* KASAN */

#define kasan_panic(FMT, ...)                                                  \
  do {                                                                         \
    kprintf("========KernelAddressSanitizer========\n");                       \
    kprintf("ERROR:\n");                                                       \
    kprintf(FMT "\n", ##__VA_ARGS__);                                          \
    kprintf("======================================\n");                       \
    panic_fail();                                                              \
  } while (0)

#define KASAN_SHADOW_SCALE_SHIFT 3
#define KASAN_SHADOW_SCALE_SIZE (1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK (KASAN_SHADOW_SCALE_SIZE - 1)

#define __MD_VIRTUAL_SHIFT 26
#define __MD_SHADOW_SIZE                                                       \
  (1ULL << (__MD_VIRTUAL_SHIFT - KASAN_SHADOW_SCALE_SHIFT))
#define __MD_CANONICAL_BASE 0xC0000000

#define KASAN_OFFSET 0xD8000000
#define KASAN_MD_SHADOW_START 0xF0000000
#define KASAN_MD_SHADOW_END (KASAN_MD_SHADOW_START + __MD_SHADOW_SIZE)

static int kasan_ready;

/* The following two structures are part of internal compiler interface:
 * https://github.com/gcc-mirror/gcc/blob/master/libsanitizer/include/sanitizer/asan_interface.h
 */

struct __asan_global_source_location {
  const char *filename;
  int line_no;
  int column_no;
};

struct __asan_global {
  const void *beg;                /* The address of the global */
  size_t size;                    /* The original size of the global */
  size_t size_with_redzone;       /* The size with the redzone */
  const char *name;               /* Name as a C string */
  const char *module_name;        /* Module name as a C string */
  unsigned long has_dynamic_init; /* Does the global have dynamic initializer */
  struct __asan_global_source_location *location; /* Location of a global */
  const void *odr_indicator; /* The address of the ODR indicator symbol */
};

static const char *kasan_code_name(uint8_t code) {
  switch (code) {
    case KASAN_CODE_GLOBALS:
      return "global";
    default:
      return "unknown";
  }
}

__always_inline static inline int8_t *kasan_md_addr_to_shad(const void *addr) {
  vaddr_t va = (vaddr_t)addr;
  return (int8_t *)(KASAN_OFFSET + (va >> KASAN_SHADOW_SCALE_SHIFT));
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
  assert(addr <= __MD_CANONICAL_BASE + (1 << __MD_VIRTUAL_SHIFT));
  if (__predict_false(!kasan_ready))
    return;
  if (!kasan_md_supported(addr))
    return;

  uint8_t code = 0;
  bool valid = true;
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
    kasan_panic("* invalid access to address %p\n"
                "* %s of size %lu\n"
                "* redzone code 0x%x (%s)",
                (void *)addr, (read ? "read" : "write"), size, code,
                kasan_code_name(code));
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
  extern uintptr_t __CTOR_LIST__, __CTOR_END__;
  for (uintptr_t *ptr = &__CTOR_LIST__; ptr != &__CTOR_END__; ptr++) {
    void (*func)(void) = (void *)(*ptr);
    (*func)();
  }
}

static void kasan_fillN(uint32_t *start, uint32_t *end) {
  while (start < end)
    *start++ = 0;
}

void kasan_init(void) {
  /* Set the whole shadow memory to zero. */
  uint32_t *ptr = (uint32_t *)KASAN_MD_SHADOW_START;
  uint32_t *end = (uint32_t *)KASAN_MD_SHADOW_END;
  kasan_fillN(ptr, end);

  /* KASAN is ready to begin errror-checking. */
  kasan_ready = 1;

  /* Call constructors that will register globals (i.e. setup redzones located
   * after each global variable). */
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

/* Called at the end of every function marked as "noreturn".
 * Performs cleanup of the current stack's shadow memory to prevent false
 * positives. */
void __asan_handle_no_return(void) {
  /* HACK: Get the current $sp value */
  uintptr_t sp;
  asm volatile("move %0, $sp" : "=r"(sp));
  /* Calculate the beginning of the stack (all stacks are one-page long) */
  sp &= 0xFFFFF000;

  /* Set the correspoding shadow memory to zero. */
  uintptr_t shadow_start = (uintptr_t) kasan_md_addr_to_shad((void *)sp);
  uintptr_t shadow_end = shadow_start + (PAGESIZE >> KASAN_SHADOW_SCALE_SHIFT);
  kasan_fillN((uint32_t *)shadow_start, (uint32_t *)shadow_end);
}

void __asan_register_globals(struct __asan_global *globals, size_t n) {
  for (size_t i = 0; i < n; i++)
    kasan_mark(globals[i].beg, globals[i].size, globals[i].size_with_redzone,
               0xFF);
}

void __asan_unregister_globals(struct __asan_global *globals, size_t n) {
  /* never called */
}

/* KASAN-replacements of various memory-touching functions */

void *kasan_memcpy(void *dst, const void *src, size_t len) {
  kasan_shadow_check((unsigned long)src, len, true);
  kasan_shadow_check((unsigned long)dst, len, false);
  return __builtin_memcpy(dst, src, len);
}

size_t kasan_strlen(const char *str) {
  const char *s = str;
  while (1) {
    kasan_shadow_check((unsigned long)s, 1, true);
    if (*s == '\0')
      break;
    s++;
  }

  return (s - str);
}
