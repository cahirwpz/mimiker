#ifndef _AARCH64_KASAN_H_
#define _AARCH64_KASAN_H_

#include <aarch64/vm_param.h>

/* Part of internal compiler interface */
#define KASAN_SHADOW_SCALE_SHIFT 3
#define KASAN_SHADOW_SCALE_SIZE (1 << KASAN_SHADOW_SCALE_SHIFT)

/* Shadow memory */
#define KASAN_MD_SHADOW_START 0xffffff0000000000
#define KASAN_MD_SHADOW_SIZE (1ULL << 30) /* 1 GB */

/* Sanitized memory (accesses within this range are checked) */
#define KASAN_MD_SANITIZED_START KERNEL_SPACE_BEGIN
#define KASAN_MD_SANITIZED_SIZE                                                \
  (KASAN_MD_SHADOW_SIZE << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_MD_SANITIZED_END                                                 \
  (KASAN_MD_SANITIZED_START + KASAN_MD_SANITIZED_SIZE)

/* Note: this offset has also to be explicitly set in CFLAGS_KASAN */
#define KASAN_MD_OFFSET                                                        \
  (KASAN_MD_SHADOW_START -                                                     \
   (KASAN_MD_SANITIZED_START >> KASAN_SHADOW_SCALE_SHIFT))

__always_inline static inline int8_t *kasan_md_addr_to_shad(uintptr_t addr) {
  return (int8_t *)(KASAN_MD_OFFSET + (addr >> KASAN_SHADOW_SCALE_SHIFT));
}

__always_inline static inline bool kasan_md_addr_supported(uintptr_t addr) {
  return addr >= KASAN_MD_SANITIZED_START && addr < KASAN_MD_SANITIZED_END;
}

#endif /* !_AARCH64_KASAN_H_ */
