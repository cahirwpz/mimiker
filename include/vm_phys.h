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

void vm_phys_add_seg(vm_paddr_t start, vm_paddr_t end);
void vm_phys_init();
vm_page_t *vm_phys_alloc(size_t order);
void vm_phys_free(struct vm_page *page);
void vm_phys_print_free_pages();
void vm_phys_reserve(vm_paddr_t start, vm_paddr_t end);

#endif /* _VM_PHYS_H */
