#ifndef _VM_PHYS_H_
#define _VM_PHYS_H_

#include <vm.h>

/* Initialize physical memory manager. */
void pm_init();

/* 
 * This adds segment to be managed by the vm_phys subsystem. 
 * In this function system uses kernel_sbrk function to allocate some data to 
 * manage pages. After kernel_sbrk_shutdown this function shouldn't be used.   
 * Note that this system manages PHYSICAL PAGES. Therefore start and end,
 * should be physical addresses. vm_offset determines initial virt_addr
 * for every page allocated from system. All pages in this segment will have
 * their default vm_addresses in range (start + vm_offset, end + vm_offset).
 */
void pm_add_segment(pm_addr_t start, pm_addr_t end, vm_addr_t vm_offset);

/* Allocates page from subsystem. Number of bytes of page is given by
 * (1 << order) * PAGESIZE. Maximal order is 16.  */
vm_page_t *pm_alloc(size_t order);

void pm_free(vm_page_t *page);
void pm_dump();

/* After using this function pages in range (start, end) are never going to be
 * allocated. Should be used at start to avoid allocating from text, data,
 * ebss, or any possibly unwanted places. */
void pm_reserve(pm_addr_t start, pm_addr_t end);

#endif /* _VM_PHYS_H */

