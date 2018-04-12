#include <kbss.h>
#include <stdc.h>
#include <vm.h>

/* Leave synchronization markers in case we need it. */
#define cs_enter()
#define cs_leave()

extern uint8_t __bss[];
/* The end of the kernel's .bss section. Provided by the linker. */
extern uint8_t __ebss[];

/*
 * The end of the memory area occupied by the kernel image.
 * Calls to `kernel_kbss` expand this area, so the value of
 * this variable may change at runtime.
 * Therefore, it is CRITICAL that all calls to `kernel_kbss`
 * occur BEFORE running any code that assumes the value of
 * `kernel_end` to be constant.
 */
static void *kernel_end = (void *)__ebss;

static struct {
  /* Pointer to free memory used to service allocation requests. */
  void *ptr;
  /* Allocation limit -- initially NULL, can be set only once. */
  void *end;
} kbss = {__ebss, NULL};

void kbss_init(void) {
  bzero(__bss, __ebss - __bss);
}

void *kbss_grow(size_t size) {
  cs_enter();
  void *ptr = kbss.ptr;
  size = roundup(size, sizeof(uint64_t));
  if (kbss.end != NULL) {
    assert(ptr + size <= kbss.end);
  } else {
    kernel_end += size;
  }
  kbss.ptr += size;
  cs_leave();
  bzero(ptr, size);
  return ptr;
}

void *kbss_fix(void) {
  void *limit = align(kernel_end, PAGESIZE);
  /* The limit can be set only once. */
  assert(kbss.end == NULL);
  assert(kbss.ptr <= limit);
  kbss.end = limit;
  kernel_end = limit;
  return kernel_end;
}
