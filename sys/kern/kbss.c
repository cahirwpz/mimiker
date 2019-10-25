#include <sys/mimiker.h>
#include <sys/kbss.h>
#include <sys/libkern.h>
#include <sys/vm.h>

/* The end of the kernel's .bss section. Provided by the linker. */
extern uint8_t __ebss[];

static struct {
  /* Pointer to free memory used to service allocation requests. */
  void *ptr;
  /* Allocation limit -- initially NULL, can be set only once. */
  void *end;
} kbss = {__ebss, NULL};

void *kbss_grow(size_t size) {
  void *ptr = kbss.ptr;
  size = roundup(size, sizeof(uint64_t));
  if (kbss.end != NULL)
    assert(ptr + size <= kbss.end);
  kbss.ptr += size;
  bzero(ptr, size);
  return ptr;
}

void *kbss_fix(void) {
  assert(kbss.end == NULL);
  kbss.end = align(kbss.ptr, PAGESIZE);
  return kbss.end;
}
