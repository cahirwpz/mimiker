#include <sys/vm.h>
#include <sys/pmap.h>
#include <sys/param.h>
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/thread.h>
#include <sys/vm_physmem.h>
#include <sys/vm.h>

/* Part of internal compiler interface */
#define KASAN_ALLOCA_REDZONE_SIZE 32

#define KASAN_SHADOW_MASK (KASAN_SHADOW_SCALE_SIZE - 1)

/* Sanitized memory (accesses within this range are checked) */
#define KASAN_MAX_SANITIZED_SIZE                                               \
  (KASAN_MAX_SHADOW_SIZE << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_MAX_SANITIZED_END                                                \
  (KASAN_SANITIZED_START + KASAN_MAX_SANITIZED_SIZE)

/* NOTE: this offset has also to be explicitly set in `CFLAGS_KASAN` */
#define KASAN_OFFSET                                                           \
  (KASAN_SHADOW_START - (KASAN_SANITIZED_START >> KASAN_SHADOW_SCALE_SHIFT))

/* Non-instrumented functions in mimiker and libkern. */
int __real_copyin(const void *restrict udaddr, void *restrict kaddr,
                  size_t len);
int __real_copyinstr(const void *restrict udaddr, void *restrict kaddr,
                     size_t len, size_t *restrict lencopied);
int __real_copyout(const void *restrict kaddr, void *restrict udaddr,
                   size_t len);
void __real_bcopy(const void *src, void *dst, size_t n);
void __real_bzero(void *dst, size_t n);
void *__real_memcpy(void *dst, const void *src, size_t n);
void *__real_memmove(void *dst, const void *src, size_t n);
void *__real_memset(void *dst, int c, size_t n);
size_t __real_strlen(const char *s);

/*
 * The following two structures are part of internal compiler interface:
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
static void assume_within_shadow_byte(uintptr_t addr, size_t size) {
  if ((addr ^ (addr + size - 1)) >> KASAN_SHADOW_SCALE_SHIFT) {
    panic("KASAN: unaligned access to shadow map, address %p, size %lu!\n",
          (void *)addr, size);
  }
}

__always_inline static inline int8_t *addr_to_shad(uintptr_t addr) {
  return (int8_t *)(KASAN_OFFSET + (addr >> KASAN_SHADOW_SCALE_SHIFT));
}

__always_inline static inline bool addr_supported(uintptr_t addr) {
  return addr >= KASAN_SANITIZED_START && addr < KASAN_MAX_SANITIZED_END;
}

__always_inline static inline uint8_t shadow_1byte_isvalid(uintptr_t addr) {
  int8_t shadow_val = *addr_to_shad(addr);
  int8_t last = addr & KASAN_SHADOW_MASK;
  return (shadow_val == 0 || last < shadow_val) ? 0 : shadow_val;
}

__always_inline static inline uint8_t shadow_2byte_isvalid(uintptr_t addr) {
  assume_within_shadow_byte(addr, 2);

  int8_t shadow_val = *addr_to_shad(addr);
  int8_t last = (addr + 1) & KASAN_SHADOW_MASK;
  return (shadow_val == 0 || last < shadow_val) ? 0 : shadow_val;
}

__always_inline static inline uint8_t shadow_4byte_isvalid(uintptr_t addr) {
  assume_within_shadow_byte(addr, 4);

  int8_t shadow_val = *addr_to_shad(addr);
  int8_t last = (addr + 3) & KASAN_SHADOW_MASK;
  return (shadow_val == 0 || last < shadow_val) ? 0 : shadow_val;
}

__always_inline static inline uint8_t shadow_8byte_isvalid(uintptr_t addr) {
  assume_within_shadow_byte(addr, 8);

  int8_t shadow_val = *addr_to_shad(addr);
  int8_t last = (addr + 7) & KASAN_SHADOW_MASK;
  return (shadow_val == 0 || last < shadow_val) ? 0 : shadow_val;
}

__always_inline static inline uint8_t shadow_Nbyte_isvalid(uintptr_t addr,
                                                           size_t size) {
  for (size_t i = 0; i < size; i++) {
    int8_t shadow_val = shadow_1byte_isvalid(addr + i);
    if (__predict_false(shadow_val))
      return shadow_val;
  }
  return 0;
}

__noreturn static void kasan_panic(uintptr_t addr, size_t size, bool read,
                                   uint8_t code) {
  panic("===========KernelAddressSanitizer===========\n"
        "* invalid access to address %p\n"
        "* %s of size %lu\n"
        "* redzone code %02x (%s)\n"
        "============================================",
        (void *)addr, (read ? "read" : "write"), size, code, code_name(code));
}

__always_inline static inline uint8_t shadow_isvalid(uintptr_t addr,
                                                     size_t size) {
  if (__builtin_constant_p(size)) {
    if (size == 1)
      return shadow_1byte_isvalid(addr);
    if (size == 2)
      return shadow_2byte_isvalid(addr);
    if (size == 4)
      return shadow_4byte_isvalid(addr);
    if (size == 8)
      return shadow_8byte_isvalid(addr);
  }
  return shadow_Nbyte_isvalid(addr, size);
}

__always_inline static inline void shadow_check(uintptr_t addr, size_t size,
                                                bool read) {
  if (__predict_false(!kasan_ready))
    return;
  if (__predict_false(!addr_supported(addr)))
    return;

  uint8_t code = shadow_isvalid(addr, size);
  if (__predict_false(code))
    kasan_panic(addr, size, read, code);
}

/*
 * Marking memory has limitations captured by assertions in the code below.
 *
 * Memory is divided into 8-byte blocks aligned to 8-byte boundary. Each block
 * has corresponding descriptor byte in the shadow memory. You can mark each
 * block as valid (0x00) or invalid (0xF1 - 0xFF). Blocks can be partially valid
 * (0x01 - 0x07) - i.e. prefix is valid, suffix is invalid.  Other variants are
 * NOT POSSIBLE! Thus `addr` and `total` must be block aligned.
 */
void kasan_mark(const void *addr, size_t valid, size_t total, uint8_t code) {
  assert(is_aligned(addr, KASAN_SHADOW_SCALE_SIZE));
  assert(is_aligned(total, KASAN_SHADOW_SCALE_SIZE));
  assert(valid <= total);

  int8_t *shadow = addr_to_shad((uintptr_t)addr);
  int8_t *end = shadow + total / KASAN_SHADOW_SCALE_SIZE;

  /* Valid bytes. */
  size_t len = valid / KASAN_SHADOW_SCALE_SIZE;
  __real_memset(shadow, 0, len);
  shadow += len;

  /* At most one partially valid byte. */
  if (valid & KASAN_SHADOW_MASK)
    *shadow++ = valid & KASAN_SHADOW_MASK;

  /* Invalid bytes. */
  if (shadow < end)
    __real_memset(shadow, code, end - shadow);
}

void kasan_mark_valid(const void *addr, size_t size) {
  kasan_mark(addr, size, size, 0);
}

void kasan_mark_invalid(const void *addr, size_t size, uint8_t code) {
  kasan_mark(addr, 0, size, code);
}

/* Call constructors that will register globals */
typedef void (*ctor_t)(void);

static void call_ctors(void) {
  extern ctor_t __CTOR_LIST__[], __CTOR_END__[];
  for (ctor_t *ctor = __CTOR_LIST__; ctor != __CTOR_END__; ctor++) {
    (*ctor)();
  }
}

void kasan_grow(vaddr_t maxkvaddr) {
  assert(mtx_owned(&vm_kernel_end_lock));
  maxkvaddr = roundup2(maxkvaddr, PAGESIZE * KASAN_SHADOW_SCALE_SIZE);
  assert(maxkvaddr < KASAN_MAX_SANITIZED_END);
  vaddr_t va = (vaddr_t)addr_to_shad(_kasan_sanitized_end);
  vaddr_t end = (vaddr_t)addr_to_shad(maxkvaddr);

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
  kasan_mark_valid((const void *)KASAN_SANITIZED_START,
                   _kasan_sanitized_end - KASAN_SANITIZED_START);

  /* KASAN is ready to check for errors! */
  kasan_ready = 1;

  /* Setup redzones after global variables */
  call_ctors();
}

#define DEFINE_ASAN_LOAD_STORE(size)                                           \
  void __asan_load##size##_noabort(uintptr_t addr) {                           \
    shadow_check(addr, size, true);                                            \
  }                                                                            \
  __weak_alias(__asan_load##size##_noabort,                                    \
               __asan_report_load##size##_noabort);                            \
  void __asan_store##size##_noabort(uintptr_t addr) {                          \
    shadow_check(addr, size, false);                                           \
  }                                                                            \
  __weak_alias(__asan_store##size##_noabort,                                   \
               __asan_report_store##size##_noabort);

DEFINE_ASAN_LOAD_STORE(1);
DEFINE_ASAN_LOAD_STORE(2);
DEFINE_ASAN_LOAD_STORE(4);
DEFINE_ASAN_LOAD_STORE(8);
DEFINE_ASAN_LOAD_STORE(16);

void __asan_loadN_noabort(uintptr_t addr, size_t size) {
  shadow_check(addr, size, true);
}

__weak_alias(__asan_loadN_noabort, __asan_report_load_n_noabort);

void __asan_storeN_noabort(uintptr_t addr, size_t size) {
  shadow_check(addr, size, false);
}

__weak_alias(__asan_storeN_noabort, __asan_report_store_n_noabort);

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
int __wrap_copyin(const void *restrict udaddr, void *restrict kaddr,
                  size_t len) {
  __asan_storeN_noabort((uintptr_t)kaddr, len);
  return __real_copyin(udaddr, kaddr, len);
}

int __wrap_copyinstr(const void *restrict udaddr, void *restrict kaddr,
                     size_t len, size_t *restrict lencopied) {
  __asan_storeN_noabort((uintptr_t)kaddr, len);
  return __real_copyinstr(udaddr, kaddr, len, lencopied);
}

int __wrap_copyout(const void *restrict kaddr, void *restrict udaddr,
                   size_t len) {
  __asan_loadN_noabort((uintptr_t)kaddr, len);
  return __real_copyout(kaddr, udaddr, len);
}

void __wrap_bcopy(const void *src, void *dst, size_t n) {
  __asan_loadN_noabort((uintptr_t)src, n);
  __asan_storeN_noabort((uintptr_t)dst, n);
  __real_bcopy(src, dst, n);
}

void __wrap_bzero(void *dst, size_t n) {
  __asan_storeN_noabort((uintptr_t)dst, n);
  __real_bzero(dst, n);
}

void *__wrap_memcpy(void *dst, const void *src, size_t n) {
  __asan_loadN_noabort((uintptr_t)src, n);
  __asan_storeN_noabort((uintptr_t)dst, n);
  return __real_memcpy(dst, src, n);
}

void *__wrap_memmove(void *dst, const void *src, size_t n) {
  __asan_loadN_noabort((uintptr_t)src, n);
  __asan_storeN_noabort((uintptr_t)dst, n);
  return __real_memmove(dst, src, n);
}

void *__wrap_memset(void *dst, int c, size_t n) {
  __asan_storeN_noabort((uintptr_t)dst, n);
  return __real_memset(dst, c, n);
}

/* Wrapper for strlen is required as long as __real_strlen is implemented in
 * assembly, i.e. for AArch64 and MIPS. */
size_t __wrap_strlen(const char *str) {
  const char *s = str;

  while (1) {
    __asan_load1_noabort((uintptr_t)s);
    if (*s == '\0')
      return s - str;
    s++;
  }
}
