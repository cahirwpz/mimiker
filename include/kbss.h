#ifndef _SYS_KBSS_H_
#define _SYS_KBSS_H_

#include <common.h>

/* Clears kernel bss section. */
void kbss_init(void);

/*
 * This function provides simple dynamic memory allocation that may be used
 * before any memory management has been initialised. This is useful, because
 * sometimes it is needed to dynamically allocate memory for some data
 * structures that are in use even before virtual memory management is ready.
 * This function does it be pretending that the kernel's .bss section ends
 * further than it originally did. In order to allocate N bytes of memory, it
 * pushes the pointer to the end of kernel's image by N bytes. This way such
 * dynamically allocated structures are appended to the memory already occupied
 * by kernel data. The extended end of kernel image will be returned by \a
 * kbss_fix, the physical memory manager will take it into account when started.
 *
 * The returned pointer is word-aligned. The block is filled with 0's.
 */
void *kbss_grow(size_t size) __warn_unused;

/* Fixes size of kernel bss section and allocation limit for \a kbss_grow. */
void *kbss_fix(void);

#endif /* _SYS_KBSS_H_ */
