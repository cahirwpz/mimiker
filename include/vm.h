#ifndef _VIRT_MEM_H_
#define _VIRT_MEM_H_

#include <common.h>
#include <queue.h>
#include <tree.h>

#define PAGESIZE 4096

#define PG_SIZE(x) (PAGESIZE << (x)->order)
#define PG_START(pg) ((pg)->phys_addr)
#define PG_END(pg) ((pg)->phys_addr + (1 << (pg->order)) * PAGESIZE)
#define PG_VADDR_START(pg) ((pg)->virt_addr)
#define PG_VADDR_END(pg) ((pg)->virt_addr+ (1 << (pg->order)) * PAGESIZE)

#define PG_VALID 0x1
#define PG_DIRTY 0x2

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
  vm_addr_t virt_addr; /* vm address in kseg0 */
  uint8_t vm_flags; /* state of page (valid or dirty) */
  uint8_t pm_flags; /* flags used by pm system */
  TAILQ_ENTRY(vm_page) freeq; /* entry in free queue */
  pm_addr_t phys_addr; /* physical address of page */
  size_t order; /* order of page */
} vm_page_t;

#endif /* _VIRT_MEM_H_ */
