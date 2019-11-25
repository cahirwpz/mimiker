#ifndef _SYS_KMEM_FLAGS_H_
#define _SYS_KMEM_FLAGS_H_

/* Common flags for kernel memory allocators (kmem, pool, kmalloc) */
#define M_WAITOK 0x0000 /* always returns memory block, but can sleep */
#define M_NOWAIT 0x0001 /* may return NULL, but cannot sleep */
#define M_ZERO 0x0002   /* clear allocated block */

#endif /* !_SYS_KMEM_FLAGS_H_ */
