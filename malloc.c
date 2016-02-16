#include <libkern.h>
#include <global_config.h>
#include <malloc.h>
#include <mutex.h>

/* The end of the kernel's .bss section. Provided by the linker. */
extern uint8_t __ebss[];
/* Initial stack pointer at the end of RAM. Provided by the linker. */
extern uint8_t _estack[];

static struct {
  void *ptr;     /* Pointer to the end of kernel's bss. */
  void *end;     /* Limit for the end of kernel's bss. */
  mtx_t lock;
  bool shutdown;
} sbrk = { __ebss, _estack - PAGESIZE, MTX_INITIALIZER, false };

void *kernel_sbrk(size_t size) {
  mtx_lock(sbrk.lock);
  void *ptr = sbrk.ptr;
  size = roundup(size, sizeof(intptr_t));
  if (ptr + size > sbrk.end)
    kprintf("%s: run out of memory\n", __func__);
  sbrk.ptr += size;
  mtx_unlock(sbrk.lock);
  bzero(ptr, size);
  return ptr;
}

void *kernel_sbrk_shutdown() {
  if (!sbrk.shutdown) {
    mtx_lock(sbrk.lock);
    sbrk.end = ALIGN(sbrk.ptr, PAGESIZE);
    sbrk.shutdown = true;
    mtx_unlock(sbrk.lock);
  }

  return sbrk.end;
}
