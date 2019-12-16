#ifndef _SYS_KASAN_H_
#define _SYS_KASAN_H_

void kasan_init(void);
void kasan_shadow_map(void *addr, size_t size);

#endif /* !_SYS_KASAN_H_ */
