#ifndef _SYS_KASAN_H_
#define _SYS_KASAN_H_

void kasan_init(void);
void kasan_mark(const void *addr, size_t size, size_t sz_with_redz,
                uint8_t code);

#endif /* !_SYS_KASAN_H_ */
