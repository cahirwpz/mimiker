#ifndef _SYS_KASAN_H_
#define _SYS_KASAN_H_

/* The following codes are part of internal compiler interface:
 * https://github.com/gcc-mirror/gcc/blob/master/libsanitizer/asan/asan_internal.h
 */
#define KASAN_CODE_STACK_LEFT 0xF1
#define KASAN_CODE_STACK_MID 0xF2
#define KASAN_CODE_STACK_RIGHT 0xF3

/* Our own redzone codes */
#define KASAN_CODE_GLOBAL 0xFA
#define KASAN_CODE_USE_AFTER_FREE 0xFB

#ifdef KASAN
void kasan_init(void);
void kasan_mark(const void *addr, size_t size, size_t sz_with_redz,
                uint8_t code);
#else
#define kasan_init() __nothing
#define kasan_mark(addr, size, sz_with_redz, code) __nothing
#endif /* !KASAN */

#endif /* !_SYS_KASAN_H_ */
