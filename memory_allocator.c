//#include <common.h>
//#include "libkern.h"
#include <stdint.h>
#include "queue.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
//#include "include/memory_allocator.h"




#define MB_MAGIC 0xC0DECAFE
#define ALIGNMENT 8

TAILQ_HEAD(mb_list, mem_block);
TAILQ_HEAD(mp_arena, mem_arena);

typedef struct mem_block {
    uint32_t mb_magic;               /* if overwritten report a memory corruption error */
    uint32_t mb_size;                /* size < 0 => free, size > 0 => alloc'd */
    uint16_t mb_flags;
    TAILQ_ENTRY(mem_block) mb_list;
    uint8_t mb_data[0];
} mem_block_t;

#define MB_UNIT sizeof(mem_block_t)

#if 0  // TODO
#define MA_SINGLE 0 /* Single large block allocated within arena. */
#define MA_BLOCKS 1 /* Arena provides space for small blocks. */

/* Allocation larger than MA_THRESHOLD will be handled by allocation of single arena. */
#define MA_THRESHOLD (2 * VM_PAGESIZE)
#endif

typedef struct mem_arena {
    TAILQ_ENTRY(mem_arena) ma_list;
    uint16_t ma_pages; /* Size in pages. */
    uint32_t ma_size; /* Size of all the blocks inside combined */
    struct mb_list ma_freeblks;
    struct mb_list ma_usedblks;
} __attribute__((aligned(MB_UNIT))) mem_arena_t;

/* Flags to malloc */
//#define M_WAITOK    0x0000 /* ignore for now */
//#define M_NOWAIT    0x0001 /* ignore for now */
#define M_ZERO      0x0002 /* clear allocated block */

typedef struct malloc_pool {
    SLIST_ENTRY(malloc_pool) mp_next;  /* Next in global chain. */
    uint32_t mp_magic;                 /* Detect programmer error. */
    const char *mp_desc;               /* Printable type name. */
    struct mp_arena mp_arena; /* First managed arena. */
} malloc_pool_t;

/* Defines a local pool of memory for use by a subsystem. */
#define MALLOC_DEFINE(pool, desc)     \
    malloc_pool_t pool[1] = {         \
        { NULL, MB_MAGIC, desc, NULL } \
    };

#define MALLOC_DECLARE(pool) \
    extern malloc_pool_t pool[1]

void malloc_add_arena(malloc_pool_t *mp, void *start, size_t size);
void* malloc2(size_t size, malloc_pool_t *mp, uint16_t flags);
//void *realloc(void *addr, size_t size, malloc_pool_t *mp, uint16_t flags);
void free2(void *addr, malloc_pool_t *mp);


// DEBUG
malloc_pool_t * global_mp;
// ENDO F DEBUG


void print_free_blocks(malloc_pool_t *mp);



void merge_right(struct mb_list *ma_freeblks, mem_block_t *mb)
{
  mem_block_t *next = TAILQ_NEXT(mb, mb_list);

  if (!next)
    return;

  char *mb_ptr = (char *)mb;
  if (mb_ptr + mb->mb_size + sizeof(mem_block_t) == (char *) next) {
    printf("Removing a block at address %p\n", next);
    TAILQ_REMOVE(ma_freeblks, next, mb_list);
    mb->mb_size += next->mb_size + sizeof(mem_block_t);
  }
}

void add_free_memory_block(mem_arena_t* ma, mem_block_t* mb, size_t total_size)
{
  //printf("Adding a free memory block of size %zu\n", total_size);
  memset(mb, 0, sizeof(mem_block_t));
  mb->mb_magic = MB_MAGIC;
  mb->mb_size = total_size - sizeof(mem_block_t);
  mb->mb_flags = 0;

  // If it's the first block, we simply add it.
  if (TAILQ_EMPTY(&ma->ma_freeblks)) {
   // printf("before:\n");
  //print_free_blocks(global_mp);
    printf("Inserting a free block with address %p\n", mb);
    TAILQ_INSERT_HEAD(&ma->ma_freeblks, mb, mb_list);
  //  printf("after:\n");

  //print_free_blocks(global_mp);

    return;
  }

  // It's not the first block, so we insert it in a sorted fashion.
  mem_block_t* current = NULL;
  mem_block_t* best_so_far = NULL;  /* mb can be inserted after this entry. */

  TAILQ_FOREACH(current, &ma->ma_freeblks, mb_list) {
    if (current < mb)
      best_so_far = current;
  }

  if (!best_so_far) {
    printf("Inserting a free block with address %p\n", mb);
    TAILQ_INSERT_HEAD(&ma->ma_freeblks, mb, mb_list);
    merge_right(&ma->ma_freeblks, mb);
  } else {
    //print_free_blocks(global_mp);
    printf("Inserting a free block with address %p\n", mb);
    TAILQ_INSERT_AFTER(&ma->ma_freeblks, best_so_far, mb, mb_list);
    //print_free_blocks(global_mp);
    merge_right(&ma->ma_freeblks, mb);
    merge_right(&ma->ma_freeblks, best_so_far);
  }
}

void add_used_memory_block(mem_arena_t* ma, mem_block_t* mb)
{
  printf("Adding a used memory block with address %p\n", mb);
  TAILQ_INSERT_HEAD(&ma->ma_usedblks, mb, mb_list);
}

void remove_used_memory_block(mem_arena_t* ma, mem_block_t* mb)
{
  printf("Removing a used memory block with address %p\n", mb);
  TAILQ_REMOVE(&ma->ma_usedblks, mb, mb_list);
}


void malloc_add_arena(malloc_pool_t *mp, void *start, size_t arena_size)
{
  if (arena_size < sizeof(mem_arena_t))
    return;

  memset(start, 0, sizeof(mem_arena_t));
  mem_arena_t *ma = start;

  TAILQ_INSERT_HEAD(&mp->mp_arena, ma, ma_list);
  ma->ma_pages = 0; // TODO
  ma->ma_size = arena_size - sizeof(mem_arena_t);

  // Adding the first free block.
  mem_block_t *mb = (mem_block_t*)((char*)mp + sizeof(mem_arena_t));
  size_t block_size = arena_size - sizeof(mem_arena_t);
  add_free_memory_block(ma, mb, block_size);
}

mem_block_t *find_entry(struct mb_list *mb_list, size_t total_size) {
  mem_block_t* current = NULL;
  TAILQ_FOREACH(current, mb_list, mb_list)
  {
    if (current->mb_size >= total_size)
      return current;
  }
  return NULL;
}

mem_block_t* try_allocating_in_area(mem_arena_t* ma, size_t requested_size, uint16_t flags) // TODO: REMOVE LAST PARAMETER
{
  mem_block_t *mb = find_entry(&ma->ma_freeblks, requested_size + sizeof(mem_block_t));



  if (!mb) /* No entry has enough space. */
    return NULL;

  printf("Removing a free block from the list with address %p\n", mb);
  TAILQ_REMOVE(&ma->ma_freeblks, mb, mb_list);
  size_t total_size_left = mb->mb_size - requested_size;
  if (total_size_left > sizeof(mem_block_t)) {
    mem_block_t *new_mb = (mem_block_t *)((char *)mb + requested_size + sizeof(mem_block_t));
    //new_mb->mb_size = size_left;
    //insert_free_block(&mr->sb_head, new_sb);
    //sb->size = SIZE_WITH_SUPERBLOCK(requested_size);
    print_free_blocks(global_mp);
    add_free_memory_block(ma, new_mb, total_size_left);
    //print_free_blocks(global_mp);
  }

  return mb;
}

void* malloc2(size_t size, malloc_pool_t *mp, uint16_t flags)
{
  size_t size_aligned;
  if (size%ALIGNMENT == 0)
    size_aligned = size;
  else
    size_aligned = size + ALIGNMENT - size%ALIGNMENT;

  /* Search for the first entry in the list that has enough space. */
  mem_arena_t* current = NULL;
  TAILQ_FOREACH(current, &mp->mp_arena, ma_list)
  {

    mem_block_t* ptr = try_allocating_in_area(current, size_aligned, flags);
    if (ptr)
    {
      add_used_memory_block(current, ptr);
      return ((char*)ptr) + sizeof(mem_block_t);
    }
  }

  return NULL;
}

void free2(void *addr, malloc_pool_t *mp)
{
  mem_arena_t* current = NULL;
  TAILQ_FOREACH(current, &mp->mp_arena, ma_list)
  {
    char* start = ((char*)current) + sizeof(mem_arena_t);
    if ((char*)addr >= start && (char*)addr < start + current->ma_size)
    {
      mem_block_t* mb = (mem_block_t*)((char*)addr) - sizeof(mem_block_t);
      printf("Removing a block at address %p\n", mp);
      remove_used_memory_block(current, mb);
      add_free_memory_block(current, mb, mb->mb_size);
    }
  }
}

void print_free_blocks(malloc_pool_t *mp)
{
  mem_arena_t* current_arena = NULL;
  TAILQ_FOREACH(current_arena, &mp->mp_arena, ma_list)
  {

    printf("Printing free blocks:\n");
    mem_block_t* current_block = NULL;
    TAILQ_FOREACH(current_block, &current_arena->ma_freeblks, mb_list)
    {
      printf("%d\n", current_block->mb_size);
    }

    printf("Printing used blocks:\n");
    current_block = NULL;
    TAILQ_FOREACH(current_block, &current_arena->ma_usedblks, mb_list)
    {
      printf("%d\n", current_block->mb_size);
    }


  }
  printf("\n");
}





int main()
{
  char text[] = "this is a pool for testing purposes";
  MALLOC_DEFINE(test_pool, text);

  global_mp = test_pool;

  char array[3000];
  malloc_add_arena(test_pool, (array) + ((long long)array)%8, 2000);

  //print_free_blocks(test_pool);
  void* ptr1 = malloc2(15, test_pool, 0);
  /*printf("%p\n", ptr1);
  void* ptr2 = malloc2(15, test_pool, 0);
  printf("%p\n", ptr2);*/

  //print_free_blocks(test_pool);

  //void* ptr3 = malloc2(15, test_pool, 0);



  return 0;
}