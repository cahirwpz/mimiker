#include "phys_mem.h"
#include <libkern.h>

/* Provided by the linker. */
extern const char _gp[];
extern const char _end[];

/* RAM bounds. */
#define RAM_START _gp
#define RAM_END  (_gp + RAM_SIZE)
/* The RAM area that is used by kernel data (.data, .bss etc.) */
#define KERNEL_RAM_START _gp
#define KERNEL_RAM_END   _end

/* The number of frames available in the entire RAM */
#define FRAMES_N (RAM_SIZE/FRAME_SIZE)

/* Each byte in the bitmap encodes the state of [word size]
 * frames. The extra byte is added for the case when FRAME_N is not a
 * multiply of [word size]. */
#define FRAME_BITMAP_TYPE unsigned
FRAME_BITMAP_TYPE frame_bitmap[FRAMES_N / sizeof(FRAME_BITMAP_TYPE) + 1];
#define  FRAMES_PER_BITMAP_ENTRY (sizeof(FRAME_BITMAP_TYPE) * 8)

/* These bit op macros are universally usefull. Shall we move them to
   a common header? */
#define   SET_BIT(n, b) (n |=  (1 << b) )
#define CLEAR_BIT(n, b) (n &= ~(1 << b) )
#define   GET_BIT(n, b) ((n & (1<<b)) != 0)

/* Returns 1 iff nth frame is in use, 0 otherwise. */
#define GET_FRAME_STATE(n) GET_BIT(frame_bitmap[n / FRAMES_PER_BITMAP_ENTRY], n % FRAMES_PER_BITMAP_ENTRY)

/* Forward declaration. */
void phys_mem_demo();

/* Calculates to which frame a given address belongs, or returns -1 if
   it is outside RAM. */
size_t get_frame_no(const void* p){
    const char* ptr = (const char*)p;
    /* Test whether it is a valid physical RAM address. */
    if(ptr < RAM_START || ptr > RAM_END){
        kprintf("get_frame_no failed: Queried pointer %x is outside RAM bounds.", ptr);
        return -1;
    }
    size_t offset = (char*)ptr - RAM_START;
    size_t frame_no = offset / FRAME_SIZE;
    return frame_no;
}

/* Translates a frame number into corresponding physical address. */
void* get_frame_addr(size_t n){
    return (char*)RAM_START + n*FRAME_SIZE;
}

/* This procedure marks all frames from first to last (inclusive) as
 * used (if mark != 0) or unused (if mark == 0), but setting/clearing
 * the corresponding bits of the frames bitmap. */
void mark_frames_used(size_t first, size_t last, unsigned mark){
    size_t frameno;
    /* The following is not too efficient, for example, when setting
       more than [word size] frames it would make sense to set an
       entire byte at once. */
    if(mark){
        if(DEBUG_PHYS_MEM) kprintf("Marking frames %u to %u as used.\n", first, last);
        for(frameno = first; frameno <= last; frameno++)
            SET_BIT( frame_bitmap[frameno / FRAMES_PER_BITMAP_ENTRY],
                     frameno % FRAMES_PER_BITMAP_ENTRY);
    }else{
        if(DEBUG_PHYS_MEM) kprintf("Marking frames %u to %u as unused.\n", first, last);
        for(frameno = first; frameno <= last; frameno++)
            CLEAR_BIT( frame_bitmap[frameno / FRAMES_PER_BITMAP_ENTRY],
                       frameno % FRAMES_PER_BITMAP_ENTRY);
    }
}

void init_phys_mem(){
    size_t kernel_data_start_frame = get_frame_no(KERNEL_RAM_START);
    size_t kernel_data_end_frame   = get_frame_no(KERNEL_RAM_END  );

    if(DEBUG_PHYS_MEM) {
        /* Some general info. */
        kprintf("RAM size: %u, frame size: %u, frames no: %u, frames per bitmap entry: %u\n",
                RAM_SIZE, FRAME_SIZE, FRAMES_N, FRAMES_PER_BITMAP_ENTRY);
        kprintf("Kernel RAM data (.data, .sdata, .bss, .sbss) starts at %x and ends at %x.\n",
                KERNEL_RAM_START, KERNEL_RAM_END);
        kprintf("These addresses occupy frames from %u to %u\n",
                kernel_data_start_frame, kernel_data_end_frame);
    }

    mark_frames_used(kernel_data_start_frame, kernel_data_end_frame, 1);

    // Perform the demonstration.
    if(DEBUG_PHYS_MEM) phys_mem_demo();
}

void phys_mem_print_state(){
    kprintf("Physical memory manager internal state:\n");
    /* Start from the last entry, and continue downwards. */
    int i = FRAMES_N / FRAMES_PER_BITMAP_ENTRY - 1;
    for(; i >= 0; i--){
        const void* batch_start_addr = RAM_START +  i    * FRAME_SIZE    ;
        const void* batch_end_addr   = RAM_START + (i+1) * FRAME_SIZE - 1;
        kprintf("0x%08x - 0x%08x:  %08x\n", batch_end_addr, batch_start_addr, frame_bitmap[i]);
    }
}

/* This function finds the frame at which a newly requested region
   might begin. */
size_t find_free_frames(size_t n){
    if(DEBUG_PHYS_MEM) kprintf("Looking for %u free frames.\n", n);
    /* A very simple first-fit implementation. */
    /* A more efficient version would skip many frames at once, if
     * they are all used. It would also probably keep a track of there
     * a large free region begins, to find a fit more quickly. */
    size_t current = 0;
    size_t batch_start = 0;
    size_t i = 0;
    for(i = 0; i < FRAMES_N; i++){
        if(GET_FRAME_STATE(i) == 0){
            /* Empty frame */
            current++;
            if(current == n){
                /* We found n free frames in a row! */
                return batch_start;
            }
        }else{
            /* Used frame */
            current = 0;
            batch_start = i+1;
        }
    }
    /* Apparently, no fit was found. */
    return -1;
}

void* alloc_frames(size_t n){
    /* Find a fitting region. */
    size_t where = find_free_frames(n);
    if(where == (size_t)-1){
        kprintf("Fatal error: Failed to allocate %d frames, no fit found.\n", n);
        return NULL;
    }
    /* Mark the frames as used.*/
    mark_frames_used(where, where + n - 1, 1);

    return get_frame_addr(where);
}

void free_frames(const void* p, size_t n){
    /* Check whether the pointer is frame-aligned. */
    const char* ptr = (const char*)p;
    if((ptr - RAM_START) % FRAME_SIZE != 0){
        kprintf("Fatal error: Pointer passed to free_frames (%x) is not frame-aligned.\n", p);
        // What to do now? We might use a panic() procedure.
        return;
    }
    size_t where = get_frame_no(p);
    /* Mark the frames as unused. */
    mark_frames_used(where, where + n -1, 0);
}

/* ================================================ */


/* A trivial demonstration. */
void phys_mem_demo(){

    phys_mem_print_state();

    kprintf("Allocating f1: %d frames. \n", 100000/FRAME_SIZE);
    void* f1 = alloc_frames(100000/FRAME_SIZE);
    kprintf("f1 is at %x\n", f1);

    phys_mem_print_state();

    kprintf("Allocating f2: %d frames. \n", 400000/FRAME_SIZE);
    void* f2 = alloc_frames(400000/FRAME_SIZE);
    kprintf("f2 is at %x\n", f2);

    phys_mem_print_state();

    // Prove we can access these addresses.
    memset(f2,1,400000);

    kprintf("Freeing f1\n");
    free_frames(f1,100000/FRAME_SIZE);

    phys_mem_print_state();

    kprintf("Allocating f3: %d frames. \n", 50000/FRAME_SIZE);
    // This region will only fit in the space that was freed by f1.
    void* f3 = alloc_frames(50000/FRAME_SIZE);
    kprintf("f3 is at %x\n", f3);

    phys_mem_print_state();

    kprintf("Freeing f2\n");
    free_frames(f2,400000/FRAME_SIZE);

    phys_mem_print_state();

    kprintf("Freeing f3\n");
    free_frames(f3, 50000/FRAME_SIZE);

    phys_mem_print_state();
}
