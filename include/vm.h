#ifndef _VIRT_MEM_H_
#define _VIRT_MEM_H_

#include <common.h>
#include <queue.h>
#include <tree.h>

#define PAGESIZE 4096
#define PAGE_SHIFT 12

#define PG_SIZE(pg) ((pg)->size * PAGESIZE)
#define PG_START(pg) ((pg)->paddr)
#define PG_END(pg) ((pg)->paddr + PG_SIZE(pg))
#define PG_VADDR_START(pg) ((pg)->vaddr)
#define PG_VADDR_END(pg) ((pg)->vaddr + PG_SIZE(pg))

#define PM_RESERVED 1  /* non releasable page */
#define PM_ALLOCATED 2 /* page has been allocated */
#define PM_MANAGED 4   /* a page is on a freeq */

#define VM_ACCESSED 1 /* page has been accessed since last check */
#define VM_MODIFIED 2 /* page has been modified since last check */

typedef intptr_t vm_paddr_t;
typedef intptr_t vm_offset_t;
typedef uintptr_t vm_size_t;

typedef enum {
  VM_PROT_NONE = 0,
  VM_PROT_READ = 1,
  VM_PROT_WRITE = 2,
  VM_PROT_EXEC = 4
} vm_prot_t;

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
  vm_addr_t vm_offset; /* offset to page in vm_object */
  vm_addr_t vaddr;     /* virtual address of page */
  pm_addr_t paddr;     /* physical address of page */
  vm_prot_t prot;      /* page access rights */
  uint8_t vm_flags;    /* flags used by virtual memory system */
  uint8_t pm_flags;    /* flags used by physical memory system */
  uint32_t size;       /* size of page in PAGESIZE units */
} vm_page_t;

TAILQ_HEAD(pg_list, vm_page);
typedef struct pg_list pg_list_t;

typedef struct vm_map vm_map_t;
typedef struct vm_map_entry vm_map_entry_t;
typedef struct vm_object vm_object_t;
typedef struct pager pager_t;

#endif /* _VIRT_MEM_H_ */
