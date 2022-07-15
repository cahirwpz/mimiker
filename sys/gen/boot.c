#include <sys/boot.h>
#include <libfdt/libfdt.h>

/* Last physical address used by kernel for boot memory allocation. */
__boot_data void *_bootmem_end;

__boot_data static uintptr_t _boot_sbrk;

/* Initialize boot memory allocator. */
__boot_text void boot_sbrk_init(paddr_t ebss_pa) {
  _boot_sbrk = align(ebss_pa, sizeof(long));
}

/* Allocates and clears physical memory just after kernel image end. */
__boot_text void *boot_sbrk(size_t n) {
  long *addr = (long *)_boot_sbrk;
  _boot_sbrk += align(n, sizeof(long));
  for (unsigned i = 0; i < n / sizeof(long); i++)
    addr[i] = 0;
  return addr;
}

__boot_text paddr_t boot_sbrk_align(size_t n) {
  _boot_sbrk = align(_boot_sbrk, n);
  return (paddr_t)_boot_sbrk;
}

__boot_text void boot_clear(paddr_t start, paddr_t end) {
  if (!is_aligned(start, sizeof(long)) || !is_aligned(end, sizeof(long)))
    halt();

  long *s = (long *)start;
  long *e = (long *)end;
  for (; s < e; s++)
    *s = 0;
}

/*
 * The compiler may choose to provide `bswap32` builtin as a function, which
 * will be placed in `.text` section. This is exactly what we need to avoid.
 * Please note this is only needed for little endian systems.
 */
__boot_text static inline uint32_t fdt32toh(fdt32_t u) {
  return ((((u)&0xff000000) >> 24) | (((u)&0x00ff0000) >> 8) |
          (((u)&0x0000ff00) << 8) | (((u)&0x000000ff) << 24));
}

/* Copy DTB to kernel .bss section. */
__boot_text paddr_t boot_save_dtb(paddr_t dtb) {
  const struct fdt_header *fdt = (void *)dtb;

  if (fdt32toh(fdt->magic) != FDT_MAGIC)
    halt();

  size_t n = fdt32toh(fdt->totalsize);
  uint8_t *src = (void *)dtb;
  uint8_t *dst = boot_sbrk(n);

  for (size_t i = 0; i < n; i++)
    dst[i] = src[i];

  return (paddr_t)dst;
}

__boot_text __noreturn void halt(void) {
  for (;;)
    continue;
}
