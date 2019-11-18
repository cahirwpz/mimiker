#ifndef _SYS_PHYSMEM_H_
#define _SYS_PHYSMEM_H_

#include <sys/vm.h>

typedef struct pm_seg pm_seg_t;

/* Platform independant initialization of physical memory manager. */
void pm_bootstrap(void);

/* Calculates the size of data structure that should be allocated to describe
 * segment of @size bytes. */
size_t pm_seg_space_needed(size_t size);
/*
 * Initialize data structure describing physical memory segment.
 *
 * Note that this system manages PHYSICAL PAGES. Therefore start and end,
 * should be physical addresses. @offset determines initial virtual address
 * offset for pages, i.e. all pages in this segment will be visible under
 * addresses (start + offset, end + offset) by default.
 */
void pm_seg_init(pm_seg_t *seg, paddr_t start, paddr_t end, off_t offset);
/* After using this function pages in range (start, end) are never going to be
 * allocated. Should be used at start to avoid allocating from text, data,
 * ebss, or any possibly unwanted places. */
void pm_seg_reserve(pm_seg_t *seg, paddr_t start, paddr_t end);
/* Add physical memory segment to physical memory manager. */
void pm_add_segment(pm_seg_t *seg);

/* Allocates contiguous big page that consists of n machine pages. */
vm_page_t *pm_alloc(size_t n);

/* Returns vm_page associated with frame of given address. */
vm_page_t *pm_find_page(paddr_t pa);

void pm_free(vm_page_t *page);
void pm_dump(void);
vm_page_t *pm_split_alloc_page(vm_page_t *pg);

#endif /* !_SYS_PHYSMEM_H_ */
