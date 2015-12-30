#ifndef __PHYS_MEM_H__
#define __PHYS_MEM_H__
#include <stddef.h>

/* 1kiB frames. This value is a tunable. It would make sense to match
 * frame size with virtual memory page size. */
#define FRAME_SIZE 1024

/* WiFire has 512kiB of RAM */
#define RAM_SIZE 1024*512

/* Set this to 1 if you need the physical memory manager to be
 * verbose. */
#define DEBUG_PHYS_MEM 0

/* This function initializes the physical memory manager. It prepares
 * the frame bitmap (taking the kernel data stored in ram into
 * account) and optionally displays some debug info. It has to be
 * called before any frame allocations. */
void pm_init();

/* Allocates n physical frames. The allocated region is guaranteed to
 * be continous, and will have exactly n * FRAME_SIZE bytes in
 * size. Returns pointer to the start of the allocated reqion, or -1
 * if allocation failed. */
void* pm_frames_alloc(size_t n) __attribute__((warn_unused_result));

/* Frees frames which were prevoiusly allocated with
 * alloc_frames(n). This will mark these frames as unused, making them
 * available for future allocations. p has to be the pointer to the
 * start of the allocated region, i.e. the pointer returned by the
 * corresponding alloc_frames(n). If the pointer is not frame
 * alligned, then it is clear it may not be a valid pointer returned
 * by alloc_frames(n), and in such case this function will fail. Also,
 * it is up to you to remember the number of frames allocated
 * (i.e. the size of region you wish to free), normally this
 * information gets stored somewhere within the allocated region. */
void pm_frames_free(const void* p, size_t n);

/* Displays debug information concerning the internal state of the
 * physical memory manager. Currently this displays the underlying
 * frame bitmap in a readable format. Useful for testing the
 * manager. */
void pm_state_print();

#endif // __PHYS_MEM_H__
