#ifndef _SYS_KBSS_H_
#define _SYS_KBSS_H_

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
 * by kernel data. The extended end of kernel image is stored in
 * kernel_bss_end, the physical memory manager will take it into account when
 * started.
 *
 * The returned pointer is word-aligned. The block is filled with 0's.
 */
void *kbss_grow(size_t size) __attribute__((warn_unused_result));

/*
 * Sets the final value of `kernel_end` and the allocation limit for
 * `kbss_grow`.
 */
void kbss_fix(void *limit);

/*
 * Returns the start address of the memory area occupied by the kernel image.
 */
intptr_t get_kernel_start(void);

/*
 * Returns the end address of the memory area occupied by the kernel image.
 * The value returned by this function may change at runtime due to calls to
 * `kernel_sbrk`. Therefore, it is CRITICAL that all calls to `kernel_sbrk`
 * and `kernel_sbrk_freeze` occur BEFORE running any code that assumes the
 * return value of this function to be constant over time.
 */
intptr_t get_kernel_end(void);

#endif /* _SYS_KBSS_H_ */
