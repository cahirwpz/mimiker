#ifndef _SYS_KASAN_H_
#define _SYS_KASAN_H_

/* The following codes are part of internal compiler interface:
 * https://github.com/gcc-mirror/gcc/blob/master/libsanitizer/asan/asan_internal.h
 */
#define KASAN_CODE_STACK_LEFT 0xF1
#define KASAN_CODE_STACK_MID 0xF2
#define KASAN_CODE_STACK_RIGHT 0xF3

/* Our own redzone codes */
#define KASAN_CODE_GLOBAL_OVERFLOW 0xFA
#define KASAN_CODE_KMEM_USE_AFTER_FREE 0xFB
#define KASAN_CODE_POOL_OVERFLOW 0xFC
#define KASAN_CODE_POOL_USE_AFTER_FREE 0xFD
#define KASAN_CODE_KMALLOC_OVERFLOW 0xFE
#define KASAN_CODE_KMALLOC_USE_AFTER_FREE 0xFF

#ifdef KASAN
void kasan_init(void);
void kasan_mark_valid(const void *addr, size_t size);
void kasan_mark(const void *addr, size_t size, size_t size_with_redzone,
                uint8_t code);
#else
#define kasan_init() __nothing
#define kasan_mark_valid(addr, size) __nothing
#define kasan_mark(addr, size, size_with_redzone, code) __nothing
#endif /* !KASAN */

#endif /* !_SYS_KASAN_H_ */
