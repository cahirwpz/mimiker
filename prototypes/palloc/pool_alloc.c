#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "queue.h"
#include "pool_alloc.h"

#define align_to_binary_pow(x, a) ((((x) >> (a))+1) << (a))

#define DEBUG 1

#if defined(DEBUG) && DEBUG > 0
    #define DEBUG_PRINT(text, args ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " text, \
        __FILE__, __LINE__, __func__, ##args)
#else
    #define DEBUG_PRINT(text, args ...)
#endif

#define panic(text, args ...)                                                 \
  __extension__({                                               \
    fprintf(stderr, "FATAL ERROR: %s:%d:%s(): " text, __FILE__, __LINE__, __func__, ##args);     \
    exit(EXIT_FAILURE);                                         \
  })
  
int page_size;

typedef struct pool_item {
    uint64_t pi_guard_number; //0xcafebabecafebabe by default, normally isn't changed, so if it has another value, memory corruption has taken place
    pool_slab_t *pi_slab; //pointer to slab containing this item
    uint64_t pi_data[0];
} pool_item_t;

pool_slab_t *create_slab(size_t size, void (*constructor)(void *, size_t)) {
    DEBUG_PRINT("Entering create_slab\n");
    pool_slab_t *slab = malloc(page_size); //TODO: Change this during implementation into kernel
    slab -> ph_nused = 0;
    slab -> ph_ntotal = (page_size-sizeof(pool_slab_t)-7)/(sizeof(pool_item_t)+size+1); // sizeof(pool_slab_t)+n*(sizeof(pool_item_t)+size+1)+7 <= page_size, 7 is maximum number of padding bytes for a bitmap 
    
    slab -> ph_nfree = slab -> ph_ntotal;
    slab -> ph_start = sizeof(pool_slab_t)+align_to_binary_pow(slab -> ph_ntotal, 3);
    memset(slab -> ph_bitmap, 0, slab -> ph_ntotal);
    for (int i = 0; i < slab -> ph_ntotal; i++) {
        pool_item_t *curr_pi = (pool_item_t*) ((uint64_t) slab+(slab -> ph_start)+i*(size+sizeof(pool_item_t)));
        curr_pi -> pi_slab = slab;
        curr_pi -> pi_guard_number = 0xcafebabecafebabe;
        constructor(curr_pi -> pi_data, size);
    }
    return slab;
}

void destroy_slab(pool_slab_t *slab, pool_t *pool) {
    DEBUG_PRINT("Entering destroy_slab\n");
    for (int i = 0; i < slab -> ph_ntotal; i++) {
        pool_item_t *curr_pi = (pool_item_t*) slab+(slab -> ph_start)+i*((pool -> pp_itemsize)+sizeof(pool_item_t));
        (pool -> pp_destructor) (curr_pi -> pi_data, (pool -> pp_itemsize));
    }
    free(slab);
}

void *slab_alloc(pool_slab_t *slab, size_t size) {
    uint8_t *bitmap = slab -> ph_bitmap;
    int i = 0;
    while (bitmap[i]) { //TODO: bitmap improvements
        i++;
    }
    pool_item_t *found_pi = (pool_item_t*) ((uint64_t) slab+(slab -> ph_start)+i*(size+sizeof(pool_item_t)));
    if (found_pi -> pi_guard_number != 0xcafebabecafebabe) {
        panic("memory corruption at item 0x%lx\n", (uint64_t) found_pi);
    }
    (slab -> ph_nused)++;
    (slab -> ph_nfree)--;
    bitmap[i] = 1;
    DEBUG_PRINT("Allocated an item (0x%lx) at slab 0x%lx, index %d\n", (uint64_t) found_pi -> pi_data, (uint64_t) found_pi -> pi_slab, i);
    return found_pi -> pi_data;
} 

void pool_init(pool_t *pool, size_t size, 
        void (*constructor)(void *, size_t), void (*destructor)(void *, size_t)) {
    /*
    init object and give it 1-page cache;
    */
    DEBUG_PRINT("Entering pool_init\n");
    size = align_to_binary_pow(size, 3); //Align to 64-bit (8 byte) word
    LIST_INIT(&(pool -> pp_empty_slabs));
    LIST_INIT(&(pool -> pp_full_slabs));
    LIST_INIT(&(pool -> pp_part_slabs));
    pool_slab_t *first_slab = create_slab(size, constructor);
    LIST_INSERT_HEAD(&(pool -> pp_empty_slabs), first_slab, ph_slablist);
    pool -> pp_constructor = constructor;
    pool -> pp_destructor = destructor;
    pool -> pp_itemsize = size;
    pool -> pp_nslabs = 1;
    pool -> pp_nitems = first_slab -> ph_ntotal;
    DEBUG_PRINT("Initialized new pool at 0x%lx\n", (uint64_t) pool);
}


void pool_destroy(pool_t* pool) {
    /*
    take some objects from the cache;
    destroy the objects;
    free the underlying memory;
    */

    DEBUG_PRINT("Entering pool_destroy\n");

    uint64_t *tmp = (uint64_t*) pool;

    for (size_t i = 0; i < sizeof(pool_t)/8; i++) {
        if (tmp[i] == 0xdeadbeefdeadbeef) {
            panic("double free at pool 0x%lx\n", (uint64_t) pool);
        }
    }

    DEBUG_PRINT("Destroying empty slabs\n");
    pool_slab_t *it = LIST_FIRST(&(pool -> pp_empty_slabs));
    while(it) {
        pool_slab_t *next = LIST_NEXT(it, ph_slablist);
        LIST_REMOVE(it, ph_slablist);
        destroy_slab(it, pool);
        it = next;
    }

    DEBUG_PRINT("Destroying partially filled slabs\n");
    it = LIST_FIRST(&(pool -> pp_part_slabs));
    while(it) {
        pool_slab_t *next = LIST_NEXT(it, ph_slablist);
        LIST_REMOVE(it, ph_slablist);
        destroy_slab(it, pool);
        it = next;
    }

    DEBUG_PRINT("Destroying full slabs\n");
    it = LIST_FIRST(&(pool -> pp_full_slabs));
    while(it) {
        pool_slab_t *next = LIST_NEXT(it, ph_slablist);
        LIST_REMOVE(it, ph_slablist);
        destroy_slab(it, pool);
        it = next;
    }

    for (size_t i = 0; i < sizeof(pool_t)/8; i++) {
        tmp[i] = 0xdeadbeefdeadbeef;
    }
    DEBUG_PRINT("Destroyed pool at 0x%lx\n", (uint64_t) pool);
}

void *pool_alloc(pool_t* pool, __attribute__((unused)) uint32_t flags) { //TODO: implement different flags
    /*
    if (there’s an object in the cache)
        take it (no construction required);
    else {
        allocate memory;
        construct the object;
    }
    */
    DEBUG_PRINT("Entering pool_alloc\n");

    pool_slab_t *new_slab;
    if (pool -> pp_nitems) {
        if (LIST_EMPTY(&(pool -> pp_part_slabs))) {
            new_slab = LIST_FIRST(&(pool -> pp_empty_slabs));
            LIST_REMOVE(new_slab, ph_slablist);
        } else {
            new_slab = LIST_FIRST(&(pool -> pp_part_slabs));
            LIST_REMOVE(new_slab, ph_slablist);
        }
    } else {
        new_slab = create_slab(pool -> pp_itemsize, pool -> pp_constructor);
        pool -> pp_nitems += new_slab -> ph_ntotal;
        pool -> pp_nslabs++;
        DEBUG_PRINT("Growing pool 0x%lx\n", (uint64_t) pool);
    }
    void *p = slab_alloc(new_slab, pool -> pp_itemsize);
    pool -> pp_nitems--;
    if (new_slab -> ph_nfree) {
        LIST_INSERT_HEAD(&(pool -> pp_part_slabs), new_slab, ph_slablist);
    } else {
        LIST_INSERT_HEAD(&(pool -> pp_full_slabs), new_slab, ph_slablist);
    }

    return p;
}
void pool_free(pool_t* pool, void* ptr) { //TODO: implement empty slab management
    DEBUG_PRINT("Entering pool_free\n");
    pool_item_t *curr_pi = ptr-sizeof(pool_item_t);
    if (curr_pi -> pi_guard_number != 0xcafebabecafebabe) {
        panic("memory corruption at item 0x%lx\n", (uint64_t) curr_pi);
    }
    pool_slab_t *curr_slab = curr_pi -> pi_slab;
    uint64_t index = ((uint64_t) curr_pi - (uint64_t) curr_slab - (uint64_t) (curr_slab -> ph_start))/((pool -> pp_itemsize)+sizeof(pool_item_t));
    uint8_t *bitmap = curr_slab -> ph_bitmap;
    if (!bitmap[index]) {
        panic("double free at item 0x%lx\n", (uint64_t) ptr);
    }
    
    bitmap[index] = 0;
    LIST_REMOVE(curr_slab, ph_slablist);
    (curr_slab -> ph_nused)--;
    (curr_slab -> ph_nfree)++;
    if (!(curr_slab -> ph_nused)) {
        LIST_INSERT_HEAD(&(pool -> pp_empty_slabs), curr_slab, ph_slablist);
    } else {
        LIST_INSERT_HEAD(&(pool -> pp_part_slabs), curr_slab, ph_slablist);
    }
    DEBUG_PRINT("Freed an item (0x%lx) at slab 0x%lx, index %ld\n", (uint64_t) ptr, (uint64_t) curr_slab, index);
}

__attribute__((constructor)) void initialize(void) { //does this even work?
    page_size = getpagesize();
}
