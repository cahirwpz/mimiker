#ifndef _MIPS_KASAN_H_
#define _MIPS_KASAN_H_

#include <mips/vm_param.h>

#ifndef PT_ENTRIES
/* TODO: take this value from some other file */
#define PT_ENTRIES 1024
#endif /* !PT_ENTRIES */

/* Number of PTEs used to describe KASAN's shadow memory.
 * Each PTE describes 4 MB of shadow, which corresponds to 32 MB of KSEG2. */
#define KASAN_MD_PTE_NUM 4

/* Part of internal compiler interface */
#define KASAN_SHADOW_SCALE_SHIFT 3

/* Shadow memory */
#define KASAN_MD_SHADOW_START 0xF0000000
#define KASAN_MD_SHADOW_SIZE (KASAN_MD_PTE_NUM * PT_ENTRIES * PAGESIZE)
#define KASAN_MD_SHADOW_END (KASAN_MD_SHADOW_START + KASAN_MD_SHADOW_SIZE)

/* Sanitized memory (accesses out of this range are not checked) */
#define KASAN_MD_SANITIZED_START KERNEL_SPACE_BEGIN /* beginning of KSEG2 */
#define KASAN_MD_SANITIZED_SIZE                                                \
  (KASAN_MD_SHADOW_SIZE << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_MD_SANITIZED_END                                                 \
  (KASAN_MD_SANITIZED_START + KASAN_MD_SANITIZED_SIZE)

/* Note: this offset has also to be explicitly set in CFLAGS_KASAN */
#define KASAN_MD_OFFSET                                                        \
  (KASAN_MD_SHADOW_START -                                                     \
   (KASAN_MD_SANITIZED_START >> KASAN_SHADOW_SCALE_SHIFT))

__always_inline static inline int8_t *
kasan_md_addr_to_shad(unsigned long addr) {
  return (int8_t *)(KASAN_MD_OFFSET + (addr >> KASAN_SHADOW_SCALE_SHIFT));
}

__always_inline static inline bool kasan_md_addr_supported(unsigned long addr) {
  return addr >= KASAN_MD_SANITIZED_START && addr < KASAN_MD_SANITIZED_END;
}

#endif /* !_MIPS_KASAN_H_ */
