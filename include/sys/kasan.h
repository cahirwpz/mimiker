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

/* Redzone sizes for instrumented allocators */
#define KASAN_POOL_REDZONE_SIZE 8
#define KASAN_KMALLOC_REDZONE_SIZE 8

/* TODO: czy potrzebujemy ttl i kasan_quarantine_inctime? */
/* TODO: czy chcemy przenieść kwarantannę do pliku kasan_quarantine? */
/* TODO: czy rozbić te zmiany na 2 PR? kwarantanna + reszta? */

/* Quarantine */
#define KASAN_QUARANTINE_BUFSIZE 64
#define KASAN_QUARANTINE_DEFAULT_TTL 64

/* First argument is pool's address, second is memory block's address. */
typedef void (*quarantine_free_t)(void *, void *);

typedef struct quarantine_item {
  void *qi_ptr;          /* pointer to quarantined item */
  int qi_timestamp_free; /* time when the item was freed */
} quarantine_item_t;

/* Quarantine structure.
   Locking:
    * all fields are protected by q_mtx
    * all kasan_quarantine_* functions require q_mtx to be already locked */
typedef struct quarantine {
  struct {
    quarantine_item_t items[KASAN_QUARANTINE_BUFSIZE];
    int head;               /* first unoccupied slot */
    int tail;               /* last occupied slot */
    int count;              /* number of occupied slots */
  } q_buf;                  /* cyclic buffer of items */
  quarantine_free_t q_free; /* function to free items after quarantine */
  int q_timestamp_current;  /* time measured in number of allocs & frees */
  int q_ttl;                /* how long are the items quarantined */
  void *q_pool;             /* pool from which the items come */
  mtx_t *q_mtx;             /* pool's mutex */
} quarantine_t;

/* KASAN interface */
#ifdef KASAN
void kasan_init(void);
void kasan_mark_valid(const void *addr, size_t size);
void kasan_mark(const void *addr, size_t size, size_t size_with_redzone,
                uint8_t code);
void kasan_quarantine_init(quarantine_t *q, void *pool, mtx_t *pool_mtx,
                           quarantine_free_t free, int ttl);
void kasan_quarantine_additem(quarantine_t *q, void *ptr);
void kasan_quarantine_inctime(quarantine_t *q);
void kasan_quarantine_releaseall(quarantine_t *q);
#else
#define kasan_init() __nothing
#define kasan_mark_valid(addr, size) __nothing
#define kasan_mark(addr, size, size_with_redzone, code) __nothing
#define kasan_quarantine_init(q, pool, pool_mtx, free, ttl) __nothing
#define kasan_quarantine_additem(q, ptr) __nothing
#define kasan_quarantine_inctime(q) __nothing
#define kasan_quarantine_releaseall(q) __nothing
#endif /* !KASAN */

#endif /* !_SYS_KASAN_H_ */
