#ifndef _VIRT_MEM_H_
#define _VIRT_MEM_H_

#include <common.h>
#include <queue.h>
#include <tree.h>

#define PAGESIZE 4096

#define VM_RESERVED 1
#define VM_FREE 2

#define PG_SIZE(x) (PAGESIZE << (x)->order)
#define PG_START(pg) ((pg)->phys_addr)
#define PG_END(pg) ((pg)->phys_addr + (1 << (pg->order)) * PAGESIZE)
#define PG_VADDR_START(pg) ((pg)->virt_addr)
#define PG_VADDR_END(pg) ((pg)->virt_addr+ (1 << (pg->order)) * PAGESIZE)

typedef struct vm_page {
  union {
    struct {
      TAILQ_ENTRY(vm_page) list;
      RB_ENTRY(vm_page) tree; 
    } obj;
    struct {
      TAILQ_ENTRY(vm_page) list;
    } pt;
  };

  vm_addr_t vm_offset; /* offset to page in vm_object */

  /* Vm address in kseg0 */
  vm_addr_t virt_addr;

  /* Following fields are to be used by vm_phys subsystem */
  TAILQ_ENTRY(vm_page) freeq;
  pm_addr_t phys_addr;
  size_t order;
  uint32_t flags;
} vm_page_t;

#endif /* _VIRT_MEM_H_ */
