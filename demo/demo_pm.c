#include "demos.h"
#include "phys_mem.h"

void demo_pm(){

    pm_state_print();

    kprintf("Allocating f1: %d frames. \n", 100000/FRAME_SIZE);
    void* f1 = pm_frames_alloc(100000/FRAME_SIZE);
    kprintf("f1 is at %x\n", f1);

    pm_state_print();

    kprintf("Allocating f2: %d frames. \n", 400000/FRAME_SIZE);
    void* f2 = pm_frames_alloc(400000/FRAME_SIZE);
    kprintf("f2 is at %x\n", f2);

    pm_state_print();

    // Prove we can access these addresses.
    memset(f2,1,400000);

    kprintf("Freeing f1\n");
    pm_frames_free(f1,100000/FRAME_SIZE);

    pm_state_print();

    kprintf("Allocating f3: %d frames. \n", 50000/FRAME_SIZE);
    // This region will only fit in the space that was freed by f1.
    void* f3 = pm_frames_alloc(50000/FRAME_SIZE);
    kprintf("f3 is at %x\n", f3);

    pm_state_print();

    kprintf("Freeing f2\n");
    pm_frames_free(f2,400000/FRAME_SIZE);

    pm_state_print();

    kprintf("Freeing f3\n");
    pm_frames_free(f3, 50000/FRAME_SIZE);

    pm_state_print();
}
