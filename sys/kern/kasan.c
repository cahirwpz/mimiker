#include <sys/vm.h>
#include <sys/pmap.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/kasan.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <machine/vm_param.h>
#include <machine/kasan.h>

/* Note: use of __builtin_bzero and __builtin_memset in this file is not
 * optimal if their implementation is instrumented (i.e. not written in asm) */

/* Part of internal compiler interface */
#define KASAN_SHADOW_SCALE_SHIFT 3

#define KASAN_SHADOW_SCALE_SIZE (1 << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK (KASAN_SHADOW_SCALE_SIZE - 1)

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

static int kasan_ready;

static const char *kasan_code_name(uint8_t code) {
  switch (code) {
    case KASAN_CODE_STACK_LEFT:
    case KASAN_CODE_STACK_MID:
    case KASAN_CODE_STACK_RIGHT:
      return "stack buffer-overflow";
    case KASAN_CODE_GLOBAL:
      return "global buffer-overflow";
    case KASAN_CODE_KMEM_USE_AFTER_FREE:
      return "kmem use-after-free";
    case KASAN_CODE_POOL_USE_AFTER_FREE:
      return "pool use-after-free";
    case KASAN_CODE_STACK_USE_AFTER_RET:
      return "stack use-after-return";
    case KASAN_CODE_STACK_USE_AFTER_SCOPE:
      return "stack use-after-scope";
    case 1 ... 7:
      return "partial redzone";
    default:
      return "unknown";
  }
}

/* Check whether all bytes from range [addr, addr + size) are mapped to
 * a single shadow byte */
__always_inline static inline bool access_within_shadow_byte(uintptr_t addr,
                                                             size_t size) {
  return (addr >> KASAN_SHADOW_SCALE_SHIFT) ==
         ((addr + size - 1) >> KASAN_SHADOW_SCALE_SHIFT);
}

__always_inline static inline bool kasan_shadow_1byte_isvalid(uintptr_t addr,
                                                              uint8_t *code) {
  int8_t shadow_val = *kasan_md_addr_to_shad(addr);
  int8_t last = addr & KASAN_SHADOW_MASK;
  if (__predict_true(shadow_val == 0 || last < shadow_val))
    return true;
  *code = shadow_val;
  return false;
}

__always_inline static inline bool kasan_shadow_2byte_isvalid(uintptr_t addr,
                                                              uint8_t *code) {
  if (!access_within_shadow_byte(addr, 2))
    return kasan_shadow_1byte_isvalid(addr, code) &&
           kasan_shadow_1byte_isvalid(addr + 1, code);

  int8_t shadow_val = *kasan_md_addr_to_shad(addr);
  int8_t last = (addr + 1) & KASAN_SHADOW_MASK;
  if (__predict_true(shadow_val == 0 || last < shadow_val))
    return true;
  *code = shadow_val;
  return false;
}

__always_inline static inline bool kasan_shadow_4byte_isvalid(uintptr_t addr,
                                                              uint8_t *code) {
  if (!access_within_shadow_byte(addr, 4))
    return kasan_shadow_2byte_isvalid(addr, code) &&
           kasan_shadow_2byte_isvalid(addr + 2, code);

  int8_t shadow_val = *kasan_md_addr_to_shad(addr);
  int8_t last = (addr + 3) & KASAN_SHADOW_MASK;
  if (__predict_true(shadow_val == 0 || last < shadow_val))
    return true;
  *code = shadow_val;
  return false;
}

__always_inline static inline bool kasan_shadow_8byte_isvalid(uintptr_t addr,
                                                              uint8_t *code) {
  if (!access_within_shadow_byte(addr, 8))
    return kasan_shadow_4byte_isvalid(addr, code) &&
           kasan_shadow_4byte_isvalid(addr + 4, code);

  int8_t shadow_val = *kasan_md_addr_to_shad(addr);
  int8_t last = (addr + 7) & KASAN_SHADOW_MASK;
  if (__predict_true(shadow_val == 0 || last < shadow_val))
    return true;
  *code = shadow_val;
  return false;
}

__always_inline static inline bool
kasan_shadow_Nbyte_isvalid(uintptr_t addr, size_t size, uint8_t *code) {
  for (size_t i = 0; i < size; i++)
    if (__predict_false(!kasan_shadow_1byte_isvalid(addr + i, code)))
      return false;
  return true;
}

__always_inline static inline void kasan_shadow_check(uintptr_t addr,
                                                      size_t size, bool read) {
  if (__predict_false(!kasan_ready))
    return;
  if (__predict_false(!kasan_md_addr_supported(addr)))
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

  if (__predict_false(!valid)) {
    kprintf("==========KernelAddressSanitizer==========\n"
            "ERROR:\n"
            "* invalid access to address %p\n"
            "* %s of size %lu\n"
            "* redzone code 0x%X (%s)\n"
            "==========================================\n",
            (void *)addr, (read ? "read" : "write"), size, code,
            kasan_code_name(code));
    panic_fail();
  }
}

/* Mark first 'size' bytes as valid, and the remaining
 * (size_with_redzone - size) bytes as invalid with given code */
void kasan_mark(const void *addr, size_t size, size_t size_with_redzone,
                uint8_t code) {
  int8_t *shadow = kasan_md_addr_to_shad((uintptr_t)addr);
  size_t redzone = size_with_redzone - roundup(size, KASAN_SHADOW_SCALE_SIZE);

  assert(is_aligned(addr, KASAN_SHADOW_SCALE_SIZE));
  assert(is_aligned(redzone, KASAN_SHADOW_SCALE_SIZE));

  /* Valid part */
  size_t len = size / KASAN_SHADOW_SCALE_SIZE;
  __builtin_memset(shadow, 0, len);
  shadow += len;

  /* Partially valid part */
  if (size & KASAN_SHADOW_MASK)
    *shadow++ = size & KASAN_SHADOW_MASK;

  /* Invalid part */
  len = redzone / KASAN_SHADOW_SCALE_SIZE;
  __builtin_memset(shadow, code, len);
}

/* Mark bytes as valid */
void kasan_mark_valid(const void *addr, size_t size) {
  kasan_mark(addr, size, size, 0);
}

/* Call constructors that will register globals */
static void kasan_ctors(void) {
  extern uintptr_t __CTOR_LIST__, __CTOR_END__;
  for (uintptr_t *ptr = &__CTOR_LIST__; ptr != &__CTOR_END__; ptr++) {
    void (*func)(void) = (void (*)(void))(*ptr);
    (*func)();
  }
}

static void kasan_shadow_clean(const void *start, size_t size) {
  assert(is_aligned(start, KASAN_SHADOW_SCALE_SIZE));
  assert(is_aligned(size, KASAN_SHADOW_SCALE_SIZE));

  void *shadow = kasan_md_addr_to_shad((uintptr_t)start);
  size /= KASAN_SHADOW_SCALE_SIZE;
  __builtin_bzero(shadow, size);
}

void kasan_init(void) {
  /* Set entire shadow memory to zero */
  kasan_shadow_clean((const void *)KASAN_MD_SANITIZED_START,
                     KASAN_MD_SANITIZED_SIZE);

  /* KASAN is ready to check for errors! */
  kasan_ready = 1;

  /* Setup redzones after global variables */
  kasan_ctors();
}

#define DEFINE_ASAN_LOAD_STORE(size)                                           \
  void __asan_load##size##_noabort(uintptr_t addr) {                           \
    kasan_shadow_check(addr, size, true);                                      \
  }                                                                            \
  void __asan_store##size##_noabort(uintptr_t addr) {                          \
    kasan_shadow_check(addr, size, false);                                     \
  }

DEFINE_ASAN_LOAD_STORE(1);
DEFINE_ASAN_LOAD_STORE(2);
DEFINE_ASAN_LOAD_STORE(4);
DEFINE_ASAN_LOAD_STORE(8);

void __asan_loadN_noabort(uintptr_t addr, size_t size) {
  kasan_shadow_check(addr, size, true);
}

void __asan_storeN_noabort(uintptr_t addr, size_t size) {
  kasan_shadow_check(addr, size, false);
}

/* Called at the end of every function marked as "noreturn".
 * Performs cleanup of the current stack's shadow memory to prevent false
 * positives. */
void __asan_handle_no_return(void) {
  kstack_t *stack = &thread_self()->td_kstack;
  kasan_shadow_clean(stack->stk_base, stack->stk_size);
}

void __asan_register_globals(struct __asan_global *globals, size_t n) {
  for (size_t i = 0; i < n; i++)
    kasan_mark(globals[i].beg, globals[i].size, globals[i].size_with_redzone,
               KASAN_CODE_GLOBAL);
}

void __asan_unregister_globals(struct __asan_global *globals, size_t n) {
  /* never called */
}

/* Below you can find replacements for various memory-touching functions */

void *kasan_memcpy(void *dst, const void *src, size_t len) {
  kasan_shadow_check((uintptr_t)src, len, true);
  kasan_shadow_check((uintptr_t)dst, len, false);
  return __builtin_memcpy(dst, src, len);
}

size_t kasan_strlen(const char *str) {
  const char *s = str;
  while (1) {
    kasan_shadow_check((uintptr_t)s, 1, true);
    if (*s == '\0')
      break;
    s++;
  }

  return s - str;
}
