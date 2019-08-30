#ifndef _SYS_VM_H_
#define _SYS_VM_H_

#define PAGESIZE 4096

#ifndef __ASSEMBLER__

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <mips/mips.h>

#define PG_SIZE(pg) ((pg)->size * PAGESIZE)
#define PG_START(pg) ((pg)->paddr)
#define PG_END(pg) ((pg)->paddr + PG_SIZE(pg))
#define PG_KSEG0_ADDR(pg) (void *)(MIPS_PHYS_TO_KSEG0((pg)->paddr))

#define is_page_aligned(addr) is_aligned((addr), PAGESIZE)

#define PM_RESERVED 1  /* non releasable page */
#define PM_ALLOCATED 2 /* page has been allocated */
#define PM_MANAGED 4   /* a page is on a freeq */

#define VM_ACCESSED 1 /* page has been accessed since last check */
#define VM_MODIFIED 2 /* page has been modified since last check */

typedef enum {
  VM_PROT_NONE = 0,
  VM_PROT_READ = 1,  /* can read page */
  VM_PROT_WRITE = 2, /* can write page */
  VM_PROT_EXEC = 4   /* can execute page */
} vm_prot_t;

typedef enum {
  VM_FILE = 0,    /* map from file (default) */
  VM_ANON = 1,    /* allocated from memory */
  VM_SHARED = 2,  /* share changes */
  VM_PRIVATE = 4, /* changes are private */
  VM_FIXED = 8,   /* map addr must be exactly as requested */
  VM_STACK = 16,  /* region grows down, like a stack */
} vm_flags_t;

typedef struct vm_page vm_page_t;
TAILQ_HEAD(pg_list, vm_page);
typedef struct pg_list pg_list_t;
RB_HEAD(pg_tree, vm_page);
typedef struct pg_tree pg_tree_t;

typedef struct vm_map vm_map_t;
typedef struct vm_segment vm_segment_t;
typedef struct vm_object vm_object_t;
typedef struct vm_pager vm_pager_t;

struct vm_page {
  union {
    TAILQ_ENTRY(vm_page) freeq; /* list of free pages for buddy system */
    TAILQ_ENTRY(vm_page) pageq; /* used to group allocated pages */
    struct {
      TAILQ_ENTRY(vm_page) list;
      RB_ENTRY(vm_page) tree;
    } obj;
  };
  vm_object_t *object; /* object owning that page */
  off_t offset;        /* offset to page in vm_object */
  paddr_t paddr;       /* physical address of page */
  uint8_t vm_flags;    /* flags used by virtual memory system */
  uint8_t pm_flags;    /* flags used by physical memory system */
  uint32_t size;       /* size of page in PAGESIZE units */
};

#endif /* !__ASSEMBLER__ */

#endif /* !_SYS_VM_H_ */
