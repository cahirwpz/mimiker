#ifndef _VIRT_MEM_H_
#define _VIRT_MEM_H_

#include <common.h>
#include <queue.h>
#include <tree.h>

#define PAGESIZE 4096

#define PG_SIZE(pg) ((pg)->size * PAGESIZE)
#define PG_START(pg) ((pg)->paddr)
#define PG_END(pg) ((pg)->paddr + PG_SIZE(pg))
#define PG_VADDR_START(pg) ((pg)->vaddr)
#define PG_VADDR_END(pg) ((pg)->vaddr + PG_SIZE(pg))

#define PG_NONE   0   /* Pages cannot be accessed */
#define PG_READ   0x2 /* Same as TLB valid mask */
#define PG_WRITE  0x4 /* Same as TLB dirty mask */

#define PM_RESERVED   1  /* non releasable page */
#define PM_ALLOCATED  2  /* page has been allocated */
#define PM_MANAGED    4  /* a page is on a freeq */
#define PM_FICTITIOUS 8  /* page has no physical mapping */

typedef struct vm_page {
  union {
    TAILQ_ENTRY(vm_page) freeq; /* list of free pages for buddy system */
    struct {
      TAILQ_ENTRY(vm_page) list;
      RB_ENTRY(vm_page) tree; 
    } obj;
    struct {
      TAILQ_ENTRY(vm_page) list;
    } pt;
  };
  vm_addr_t vm_offset;          /* offset to page in vm_object */
  vm_addr_t vaddr;              /* virtual address of page */
  pm_addr_t paddr;              /* physical address of page */
  uint8_t prot;                 /* protection flags (read/write) */
  bool dirty;                   /* Has there been a write to page? */
  uint8_t pm_flags;             /* flags used by pm system */
  unsigned size;                /* size of page in PAGESIZE units */
} vm_page_t;

TAILQ_HEAD(pg_list, vm_page);
typedef struct pg_list pg_list_t;

#endif /* _VIRT_MEM_H_ */

