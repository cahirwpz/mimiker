#ifndef _SYS_KASAN_H_
#define _SYS_KASAN_H_

#define KASAN_CODE_GLOBALS 0xFA

#ifdef KASAN
void kasan_init(void);
void kasan_mark(const void *addr, size_t size, size_t sz_with_redz,
                uint8_t code);
#else
#define kasan_init() __nothing
#define kasan_mark(addr, size, sz_with_redz, code) __nothing
#endif /* KASAN */

#endif /* !_SYS_KASAN_H_ */
