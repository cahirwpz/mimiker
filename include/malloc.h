#ifndef __SYS_MALLOC_H__
#define __SYS_MALLOC_H__

#include <common.h>
#include <vm_phys.h>

/*
 * This function provides simple dynamic memory allocation that may be used
 * before any memory management has been initialised. This is useful, because
 * sometimes it is needed to dynamically allocate memory for some data
 * structures that are in use even before virtual memory management is ready.
 * This function does it be pretending that the kernel's .bss section ends
 * further than it originally did. In order to allocate N bytes of memory, it
 * pushes the pointer to the end of kernel's image by N bytes. This way such
 * dynamically allocated structures are appended to the memory already occupied
 * by kernel data. The extended end of kernel image is stored in
 * kernel_bss_end, the physical memory manager will take it into account when
 * started.
 *
 * The returned pointer is word-aligned. The block is filled with 0's.
 */
void *kernel_sbrk(size_t size) __attribute__((warn_unused_result));

/*
 * General purpose kernel memory allocator.
 */
typedef struct malloc_pool malloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(pool, desc) \
    malloc_pool_t pool[1] = {     \
        {{NULL}, MAGIC, desc}     \
    };

#define MALLOC_DECLARE(pool)      \
    extern malloc_pool_t pool[1]

/* Flags to malloc */
#define M_WAITOK    0x0000 /* ignore for now */
#define M_NOWAIT    0x0001 /* ignore for now */
#define M_ZERO      0x0002 /* clear allocated block */

void kmalloc_add_arena(malloc_pool_t *mp, vm_addr_t, size_t size);
void *kmalloc(malloc_pool_t *mp, size_t size, uint16_t flags);
void kfree(malloc_pool_t *mp, void *addr);
void kmalloc_dump(malloc_pool_t *mp);
void kmalloc_test();

#endif /* __SYS_MALLOC_H__ */
