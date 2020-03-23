#ifndef _MIPS_KASAN_H_
#define _MIPS_KASAN_H_

#include <mips/vm_param.h>
#define PT_ENTRIES 1024 /* TODO: take this value from some other file */

/* Part of internal compiler interface */
#define KASAN_SHADOW_SCALE_SHIFT 3

/* Number of PTEs used to describe KASAN's shadow memory.
 * Each PTE describes 4 MB of shadow, which corresponds to 32 MB of KSEG2. */
#define KASAN_MD_PTE_NUM 4

#define KASAN_MD_SHADOW_SIZE (KASAN_MD_PTE_NUM * PT_ENTRIES * PAGESIZE)
#define KASAN_MD_SANITIZED_SIZE                                                \
  (KASAN_MD_SHADOW_SIZE << KASAN_SHADOW_SCALE_SHIFT)

#define KASAN_MD_SANITIZED_START 0xC0000000 /* beginning of KSEG2 */
#define KASAN_MD_SANITIZED_END                                                 \
  (KASAN_MD_SANITIZED_START + KASAN_MD_SANITIZED_SIZE)

#define KASAN_MD_SHADOW_START 0xF0000000
#define KASAN_MD_SHADOW_END (KASAN_MD_SHADOW_START + KASAN_MD_SHADOW_SIZE)

#define KASAN_MD_OFFSET 0xD8000000

__always_inline static inline int8_t *kasan_md_addr_to_shad(const void *addr) {
  vaddr_t va = (vaddr_t)addr;
  return (int8_t *)(KASAN_MD_OFFSET + (va >> KASAN_SHADOW_SCALE_SHIFT));
}

__always_inline static inline bool kasan_md_addr_supported(vaddr_t addr) {
  return addr >= KASAN_MD_SANITIZED_START && addr < KASAN_MD_SANITIZED_END;
}

#endif /* !_MIPS_KASAN_H_ */
