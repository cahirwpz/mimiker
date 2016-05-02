#ifndef __MEMORY_ALLOCATOR_H__
#define __MEMORY_ALLOCATOR_H__

#include <common.h>

TAILQ_HEAD(sb_head, super_block);

typedef struct memory_range {
  void *start; /* Memory will be allocated from this address upwards. */
  size_t size; /* Number of bytes which we can use */
  struct sb_head sb_head; /* List of superblocks */
} memory_range;

typedef struct super_block {
  size_t size; /* Includes the super_block size. */
  TAILQ_ENTRY(super_block) sb_link; /* Link to other superblocks. */
} super_block;


/*
  This is a generic memory allocator. The memory that it can
  allocate from should be provided to init_memory_range().

  To allocate X usable bytes, X + sizeof(super_block) bytes will be used.
  The additional memory is the allocator's metadata.
  If allocate() returns addr, then the metadata begins at address (addr - sizeof(super_block)).
  You can use it the same was as you would use UNIX's malloc().

*/
void init_memory_range(memory_range *mr, void *start, size_t size);

void *allocate(memory_range *mr, size_t requested_size);

void deallocate(memory_range *mr, void *memory_ptr);

void test_memory_allocator();

#endif
