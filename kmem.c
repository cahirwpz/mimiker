#include "kmem.h"
#include "libkern.h"
#include "common.h"

/* The end of the kernel's .bss section. Provided by the linker. */
extern char __ebss[];

/* This is the pointer to the extended end of kernel's image.
 * It shall be always word-aligned. See km_early_alloc for details. */
static uintptr_t km_kernel_image_end = (uintptr_t)__ebss;

void* km_early_alloc(size_t size)
{
    // TODO: Explicitly crash if this function is called after
    // physical memory manager has been initialized.
    void* ptr = (void*)km_kernel_image_end;
    bzero(ptr, size);
    km_kernel_image_end = ALIGN(km_kernel_image_end + size, sizeof(word_t));
    return ptr;
}
