#include <sys/vm.h>
#include <sys/pmap.h>
#include <sys/param.h>
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/thread.h>
#include <sys/vm_physmem.h>
#include <sys/vm.h>
#include <machine/vm_param.h>
#include <machine/kasan.h>

/* Part of internal compiler interface */
#define KASAN_SHADOW_SCALE_SHIFT 3
#define KASAN_ALLOCA_REDZONE_SIZE 32

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

vaddr_t _kasan_sanitized_end;
static int kasan_ready;

static const char *code_name(uint8_t code) {
  switch (code) {
    case KASAN_CODE_STACK_LEFT:
    case KASAN_CODE_STACK_MID:
    case KASAN_CODE_STACK_RIGHT:
      return "stack buffer-overflow";
    case KASAN_CODE_GLOBAL_OVERFLOW:
      return "global buffer-overflow";
    case KASAN_CODE_KMEM_FREED:
      return "kmem use-after-free";
    case KASAN_CODE_POOL_OVERFLOW:
      return "pool buffer-overflow";
    case KASAN_CODE_POOL_FREED:
      return "pool use-after-free";
    case KASAN_CODE_KMALLOC_OVERFLOW:
      return "kmalloc buffer-overflow";
    case KASAN_CODE_KMALLOC_FREED:
      return "kmalloc use-after-free";
    case KASAN_CODE_FRESH_KVA:
      return "unused kernel address space";
    case 1 ... 7:
      return "partial redzone";
    default:
      return "unknown redzone";
  }
}

/* Check whether all bytes from range [addr, addr + size) are mapped to
 * a single shadow byte */
__always_inline static inline bool access_within_shadow_byte(uintptr_t addr,
                                                             size_t size) {
  return (addr >> KASAN_SHADOW_SCALE_SHIFT) ==
         ((addr + size - 1) >> KASAN_SHADOW_SCALE_SHIFT);
}

__always_inline static inline bool shadow_1byte_isvalid(uintptr_t addr,
                                                        uint8_t *code) {
  int8_t shadow_val = *kasan_md_addr_to_shad(addr);
  int8_t last = addr & KASAN_SHADOW_MASK;
  if (__predict_true(shadow_val == 0 || last < shadow_val))
    return true;
  *code = shadow_val;
  return false;
}

__always_inline static inline bool shadow_2byte_isvalid(uintptr_t addr,
                                                        uint8_t *code) {
  if (!access_within_shadow_byte(addr, 2))
    return shadow_1byte_isvalid(addr, code) &&
           shadow_1byte_isvalid(addr + 1, code);

  int8_t shadow_val = *kasan_md_addr_to_shad(addr);
  int8_t last = (addr + 1) & KASAN_SHADOW_MASK;
  if (__predict_true(shadow_val == 0 || last < shadow_val))
    return true;
  *code = shadow_val;
  return false;
}

__always_inline static inline bool shadow_4byte_isvalid(uintptr_t addr,
                                                        uint8_t *code) {
  if (!access_within_shadow_byte(addr, 4))
    return shadow_2byte_isvalid(addr, code) &&
           shadow_2byte_isvalid(addr + 2, code);

  int8_t shadow_val = *kasan_md_addr_to_shad(addr);
  int8_t last = (addr + 3) & KASAN_SHADOW_MASK;
  if (__predict_true(shadow_val == 0 || last < shadow_val))
    return true;
  *code = shadow_val;
  return false;
}

__always_inline static inline bool shadow_8byte_isvalid(uintptr_t addr,
                                                        uint8_t *code) {
  if (!access_within_shadow_byte(addr, 8))
    return shadow_4byte_isvalid(addr, code) &&
           shadow_4byte_isvalid(addr + 4, code);

  int8_t shadow_val = *kasan_md_addr_to_shad(addr);
  int8_t last = (addr + 7) & KASAN_SHADOW_MASK;
  if (__predict_true(shadow_val == 0 || last < shadow_val))
    return true;
  *code = shadow_val;
  return false;
}

__always_inline static inline bool
shadow_Nbyte_isvalid(uintptr_t addr, size_t size, uint8_t *code) {
  for (size_t i = 0; i < size; i++)
    if (__predict_false(!shadow_1byte_isvalid(addr + i, code)))
      return false;
  return true;
}

__always_inline static inline void shadow_check(uintptr_t addr, size_t size,
                                                bool read) {
  if (__predict_false(!kasan_ready))
    return;
  if (__predict_false(!kasan_md_addr_supported(addr)))
    return;

  uint8_t code = 0;
  bool valid = true;
  if (__builtin_constant_p(size)) {
    switch (size) {
      case 1:
        valid = shadow_1byte_isvalid(addr, &code);
        break;
      case 2:
        valid = shadow_2byte_isvalid(addr, &code);
        break;
      case 4:
        valid = shadow_4byte_isvalid(addr, &code);
        break;
      case 8:
        valid = shadow_8byte_isvalid(addr, &code);
        break;
    }
  } else {
    valid = shadow_Nbyte_isvalid(addr, size, &code);
  }

  if (__predict_false(!valid)) {
    panic("===========KernelAddressSanitizer===========\n"
          "* invalid access to address %p\n"
          "* %s of size %lu\n"
          "* redzone code %02x (%s)\n"
          "============================================",
          (void *)addr, (read ? "read" : "write"), size, code, code_name(code));
  }
}

/* Marking memory has limitations captured by assertions in the code below.
 *
 * Memory is divided into 8-byte blocks aligned to 8-byte boundary. Each block
 * has corresponding descriptor byte in the shadow memory. You can mark each
 * block as valid (0x00) or invalid (0xF1 - 0xFF). Blocks can be partially valid
 * (0x01 - 0x07) - i.e. prefix is valid, suffix is invalid.  Other variants are
 * NOT POSSIBLE! Thus `addr` and `total` must be block aligned.
 *
 * Note: use of __builtin_memset in this function is not optimal if its
 * implementation is instrumented (i.e. not written in asm). */
void kasan_mark(const void *addr, size_t valid, size_t total, uint8_t code) {
  assert(is_aligned(addr, KASAN_SHADOW_SCALE_SIZE));
  assert(is_aligned(total, KASAN_SHADOW_SCALE_SIZE));
  assert(valid <= total);

  int8_t *shadow = kasan_md_addr_to_shad((uintptr_t)addr);
  int8_t *end = shadow + total / KASAN_SHADOW_SCALE_SIZE;

  /* Valid bytes. */
  size_t len = valid / KASAN_SHADOW_SCALE_SIZE;
  __builtin_memset(shadow, 0, len);
  shadow += len;

  /* At most one partially valid byte. */
  if (valid & KASAN_SHADOW_MASK)
    *shadow++ = valid & KASAN_SHADOW_MASK;

  /* Invalid bytes. */
  if (shadow < end)
    __builtin_memset(shadow, code, end - shadow);
}

void kasan_mark_valid(const void *addr, size_t size) {
  kasan_mark(addr, size, size, 0);
}

void kasan_mark_invalid(const void *addr, size_t size, uint8_t code) {
  kasan_mark(addr, 0, size, code);
}

/* Call constructors that will register globals */
static void call_ctors(void) {
  extern uintptr_t __CTOR_LIST__, __CTOR_END__;
  for (uintptr_t *ptr = &__CTOR_LIST__; ptr != &__CTOR_END__; ptr++) {
    void (*func)(void) = (void (*)(void))(*ptr);
    (*func)();
  }
}

static inline vaddr_t kasan_va_to_shadow(vaddr_t va) {
  return KASAN_MD_SHADOW_START +
         (va - KASAN_MD_SANITIZED_START) / KASAN_SHADOW_SCALE_SIZE;
}

void kasan_grow(vaddr_t maxkvaddr) {
  assert(mtx_owned(&vm_kernel_end_lock));
  maxkvaddr = roundup2(maxkvaddr, PAGESIZE * KASAN_SHADOW_SCALE_SIZE);
  vaddr_t va = kasan_va_to_shadow(_kasan_sanitized_end);
  vaddr_t end = kasan_va_to_shadow(maxkvaddr);

  /* Allocate and map shadow pages to cover the new KVA space. */
  for (; va < end; va += PAGESIZE) {
    vm_page_t *pg = vm_page_alloc(1);
    pmap_kenter(va, pg->paddr, VM_PROT_READ | VM_PROT_WRITE, 0);
  }

  if (maxkvaddr > _kasan_sanitized_end) {
    kasan_mark_invalid((const void *)(_kasan_sanitized_end),
                       maxkvaddr - _kasan_sanitized_end, KASAN_CODE_FRESH_KVA);
    _kasan_sanitized_end = maxkvaddr;
  }
}

void init_kasan(void) {
  /* Set entire shadow memory to zero */
  kasan_mark_valid((const void *)KASAN_MD_SANITIZED_START,
                   _kasan_sanitized_end - KASAN_MD_SANITIZED_START);

  /* KASAN is ready to check for errors! */
  kasan_ready = 1;

  /* Setup redzones after global variables */
  call_ctors();
}

#define DEFINE_ASAN_LOAD_STORE(size)                                           \
  void __asan_load##size##_noabort(uintptr_t addr) {                           \
    shadow_check(addr, size, true);                                            \
  }                                                                            \
  void __asan_store##size##_noabort(uintptr_t addr) {                          \
    shadow_check(addr, size, false);                                           \
  }

DEFINE_ASAN_LOAD_STORE(1);
DEFINE_ASAN_LOAD_STORE(2);
DEFINE_ASAN_LOAD_STORE(4);
DEFINE_ASAN_LOAD_STORE(8);

void __asan_loadN_noabort(uintptr_t addr, size_t size) {
  shadow_check(addr, size, true);
}

void __asan_storeN_noabort(uintptr_t addr, size_t size) {
  shadow_check(addr, size, false);
}

/* Called at the end of every function marked as "noreturn".
 * Performs cleanup of the current stack's shadow memory to prevent false
 * positives. */
void __asan_handle_no_return(void) {
  kstack_t *stack = &thread_self()->td_kstack;
  kasan_mark_valid(stack->stk_base, stack->stk_size);
}

void __asan_register_globals(struct __asan_global *globals, size_t n) {
  for (size_t i = 0; i < n; i++)
    kasan_mark(globals[i].beg, globals[i].size, globals[i].size_with_redzone,
               KASAN_CODE_GLOBAL_OVERFLOW);
}

void __asan_unregister_globals(struct __asan_global *globals, size_t n) {
  /* never called */
}

/* Note: alloca is currently used in strntoul and test_sleepq_sync functions */
void __asan_alloca_poison(const void *addr, size_t size) {
  void *left_redzone = (int8_t *)addr - KASAN_ALLOCA_REDZONE_SIZE;
  size_t size_with_mid_redzone = roundup(size, KASAN_ALLOCA_REDZONE_SIZE);
  void *right_redzone = (int8_t *)addr + size_with_mid_redzone;

  kasan_mark_invalid(left_redzone, KASAN_ALLOCA_REDZONE_SIZE,
                     KASAN_CODE_STACK_LEFT);
  kasan_mark(addr, size, size_with_mid_redzone, KASAN_CODE_STACK_MID);
  kasan_mark_invalid(right_redzone, KASAN_ALLOCA_REDZONE_SIZE,
                     KASAN_CODE_STACK_RIGHT);
}

void __asan_allocas_unpoison(const void *begin, const void *end) {
  size_t size = end - begin;
  if (__predict_false(!begin || begin > end))
    return;
  kasan_mark_valid(begin, size);
}

/* Below you can find wrappers for various memory-touching functions,
 * which are implemented in assembly (therefore are not instrumented). */
#undef copyin
int copyin(const void *restrict udaddr, void *restrict kaddr, size_t len);
int kasan_copyin(const void *restrict udaddr, void *restrict kaddr,
                 size_t len) {
  shadow_check((uintptr_t)kaddr, len, false);
  return copyin(udaddr, kaddr, len);
}

#undef copyinstr
int copyinstr(const void *restrict udaddr, void *restrict kaddr, size_t len,
              size_t *restrict lencopied);
int kasan_copyinstr(const void *restrict udaddr, void *restrict kaddr,
                    size_t len, size_t *restrict lencopied) {
  shadow_check((uintptr_t)kaddr, len, false);
  return copyinstr(udaddr, kaddr, len, lencopied);
}

#undef copyout
int copyout(const void *restrict kaddr, void *restrict udaddr, size_t len);
int kasan_copyout(const void *restrict kaddr, void *restrict udaddr,
                  size_t len) {
  shadow_check((uintptr_t)kaddr, len, true);
  return copyout(kaddr, udaddr, len);
}

#undef memcpy
void *memcpy(void *dst, const void *src, size_t len);
void *kasan_memcpy(void *dst, const void *src, size_t len) {
  shadow_check((uintptr_t)src, len, true);
  shadow_check((uintptr_t)dst, len, false);
  return memcpy(dst, src, len);
}

size_t kasan_strlen(const char *str) {
  const char *s = str;
  while (1) {
    shadow_check((uintptr_t)s, 1, true);
    if (*s == '\0')
      break;
    s++;
  }

  return s - str;
}
