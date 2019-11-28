#ifndef _SYS_KMEM_FLAGS_H_
#define _SYS_KMEM_FLAGS_H_

/* Common flags for kernel memory allocators (kmem, pool, kmalloc) */
typedef enum {
  M_WAITOK = 0x00000000, /* always returns memory block, but can sleep */
  M_NOWAIT = 0x00000001, /* may return NULL, but cannot sleep */
  M_ZERO = 0x00000002,   /* clear allocated block */
  M_NOGROW = 0x00000004, /* don't grow mempool on alloc request, but fail */
} kmem_flags_t;

#endif /* !_SYS_KMEM_FLAGS_H_ */
