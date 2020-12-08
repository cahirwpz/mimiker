#ifndef _SYS_VM_H_
#define _SYS_VM_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <machine/vm_param.h>

#define page_aligned_p(addr) is_aligned((addr), PAGESIZE)

/* Real kernel end in kernel virtual address space. */
extern void *vm_kernel_end;

typedef enum {
  PG_ALLOCATED = 0x01,  /* page has been allocated */
  PG_MANAGED = 0x02,    /* a page is on a freeq */
  PG_REFERENCED = 0x04, /* page has been accessed since last check */
  PG_MODIFIED = 0x08,   /* page has been modified since last check */
} __packed pg_flags_t;

typedef enum {
  VM_PROT_NONE = 0,
  VM_PROT_READ = 1,  /* can read page */
  VM_PROT_WRITE = 2, /* can write page */
  VM_PROT_EXEC = 4   /* can execute page */
} vm_prot_t;

#define VM_PROT_MASK (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC)

typedef enum {
  VM_FILE = 0x0000,    /* map from file (default) */
  VM_ANON = 0x1000,    /* allocated from memory */
  VM_STACK = 0x2000,   /* region grows down, like a stack */
  VM_SHARED = 0x0001,  /* share changes */
  VM_PRIVATE = 0x0002, /* changes are private */
  VM_FIXED = 0x0004,   /* map addr must be exactly as requested */
} vm_flags_t;

typedef struct vm_page vm_page_t;
typedef TAILQ_HEAD(vm_pagelist, vm_page) vm_pagelist_t;
typedef RB_HEAD(vm_pagetree, vm_page) vm_pagetree_t;

typedef struct pv_entry pv_entry_t;
typedef struct vm_object vm_object_t;
typedef struct slab slab_t;

struct vm_page {
  union {
    TAILQ_ENTRY(vm_page) freeq; /* list of free pages for buddy system */
    TAILQ_ENTRY(vm_page) pageq; /* used to group allocated pages */
    struct {
      TAILQ_ENTRY(vm_page) list;
      RB_ENTRY(vm_page) tree;
    } obj;
    slab_t *slab; /* active when page is used by pool allocator */
  };
  TAILQ_HEAD(, pv_entry) pv_list; /* where this page is mapped? */
  vm_object_t *object;            /* object owning that page */
  off_t offset;                   /* offset to page in vm_object */
  paddr_t paddr;                  /* physical address of page */
  pg_flags_t flags;               /* page flags (used by physmem as well) */
  uint32_t size;                  /* size of page in PAGESIZE units */
};

int do_mmap(vaddr_t *addr_p, size_t length, int u_prot, int u_flags);
int do_munmap(vaddr_t addr, size_t length);

#endif /* !_SYS_VM_H_ */
