#include <sys/types.h>
#include <sys/mimiker.h>
#include <sys/kasan.h>

int kasan_ready;

void set_kasan_ready(void) {
  kasan_ready = 1;
}

void kasan_shadow_check(unsigned long addr, size_t size) {
  if (!kasan_ready)
    return;
  panic_fail();
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
