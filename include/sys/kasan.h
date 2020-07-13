#ifndef _SYS_KASAN_H_
#define _SYS_KASAN_H_

#if KASAN

#include <sys/types.h>

/* The following codes are part of internal compiler interface:
 * https://github.com/gcc-mirror/gcc/blob/master/libsanitizer/asan/asan_internal.h
 */
#define KASAN_CODE_STACK_LEFT 0xF1
#define KASAN_CODE_STACK_MID 0xF2
#define KASAN_CODE_STACK_RIGHT 0xF3

/* Our own redzone codes */
#define KASAN_CODE_GLOBAL_OVERFLOW 0xFA
#define KASAN_CODE_KMEM_FREED 0xFB
#define KASAN_CODE_POOL_OVERFLOW 0xFC
#define KASAN_CODE_POOL_FREED 0xFD
#define KASAN_CODE_KMALLOC_OVERFLOW 0xFE
#define KASAN_CODE_KMALLOC_FREED 0xFF

/* Redzone sizes for instrumented allocators */
#define KASAN_POOL_REDZONE_SIZE 8
#define KASAN_KMALLOC_REDZONE_SIZE 8

/* Quarantine */
#define KASAN_QUAR_BUFSIZE 32

/* First argument is pool's address, second is memory block's address. */
typedef void (*quar_free_t)(void *, void *);

/* Quarantine structure */
typedef struct {
  struct {
    void *items[KASAN_QUAR_BUFSIZE];
    int head;         /* first unoccupied slot */
    int tail;         /* last occupied slot */
    int count;        /* number of occupied slots */
  } q_buf;            /* cyclic buffer of items */
  quar_free_t q_free; /* function to free items after quarantine */
  void *q_pool;       /* pool from which the items come */
} quar_t;

/* Initialize KASAN subsystem.
 *
 * Should be called during early kernel boot process, as soon as the shadow
 * memory is usable. */
void init_kasan(void);

/* Mark bytes as valid (in the shadow memory) */
void kasan_mark_valid(const void *addr, size_t size);

/* Mark bytes as invalid (in the shadow memory) */
void kasan_mark_invalid(const void *addr, size_t size, uint8_t code);

/* Mark first 'size' bytes as valid (in the shadow memory), and the remaining
 * (size_with_redzone - size) bytes as invalid with given code. */
void kasan_mark(const void *addr, size_t size, size_t size_with_redzone,
                uint8_t code);

/* Initialize given quarantine structure */
void kasan_quar_init(quar_t *q, void *pool, quar_free_t free);

/* Add an item to a quarantine. */
void kasan_quar_additem(quar_t *q, void *ptr);

/* Release all items from the quarantine. */
void kasan_quar_releaseall(quar_t *q);
#else /* !KASAN */
#define init_kasan() __nothing
#define kasan_mark_valid(addr, size) __nothing
#define kasan_mark_invalid(addr, size, code) __nothing
#define kasan_mark(addr, size, size_with_redzone, code) __nothing
#define kasan_quar_init(q, pool, free) __nothing
#define kasan_quar_additem(q, ptr) __nothing
#define kasan_quar_releaseall(q) __nothing
#endif

#endif /* !_SYS_KASAN_H_ */
