#ifndef _VM_PHYS_H_
#define _VM_PHYS_H_

#include <queue.h>
#include <common.h>

#define PAGESIZE 4096
#define VM_NFREEORDER 16
#define VM_RESERVED 1
#define VM_FREE 2

typedef uintptr_t vm_addr_t;
typedef uintptr_t vm_paddr_t;

typedef struct vm_page {
  TAILQ_ENTRY(vm_page) listq;
  vm_addr_t virt_addr;
  size_t size;

  /* Following fields are to be used by vm_phys subsystem */
  TAILQ_ENTRY(vm_page) freeq;
  vm_paddr_t phys_addr;
  size_t order;
  uint32_t flags;
} vm_page_t;

TAILQ_HEAD(vm_freelist, vm_page);

typedef struct vm_phys_seg {
  TAILQ_ENTRY(vm_phys_seg) segq;
  vm_paddr_t start;
  vm_paddr_t end;
  struct vm_freelist free_queues[VM_NFREEORDER];
  struct vm_page *page_array;
} vm_phys_seg_t;


void vm_phys_init();
/* This adds segment to be managed by the vm_phys subsystem. 
 * In this function system uses kernel_sbrk function to allocate some data to 
 * manage pages. After kernel_sbrk_shutdown this function shouldn't be used.   
 * Note that this system manages PHYSICAL PAGES. Therefore start and end,
 * should be physical addresses. vm_initial_offset determines initial virt_addr
 * for every page allocated from system. All pages in this segment will have
 * their default vm_addresses in range (start+vm_initial_offset, end+vm_initial_offset). */
void vm_phys_add_seg(vm_paddr_t start, vm_paddr_t end, vm_paddr_t vm_initial_offset);

/* Allocates page from subsystem. Number of bytes of page is given by
 * (1 << order) * PAGESIZE. Maximal order is 16.  */
vm_page_t *vm_phys_alloc(size_t order);
void vm_phys_free(struct vm_page *page);
void vm_phys_print_free_pages();

/* After using this function pages in range (start, end) are never going 
 * to be allocated. Should be used at start to avoid allocating from text, data, ebss,
 * or any possibly unwanted places. */
void vm_phys_reserve(vm_paddr_t start, vm_paddr_t end);

#endif /* _VM_PHYS_H */
