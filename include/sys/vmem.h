#ifndef _SYS_VMEM_H_
#define _SYS_VMEM_H_

#include <sys/types.h>
#include <sys/kmem_flags.h>
#include <sys/mutex.h>
#include <sys/queue.h>

typedef uintptr_t vmem_addr_t;
typedef size_t vmem_size_t;

#define VMEM_MAXORDER ((int)(sizeof(vmem_size_t) * CHAR_BIT))
#define VMEM_MAXHASH 512
#define VMEM_NAME_MAX 16

typedef TAILQ_HEAD(vmem_seglist, bt) vmem_seglist_t;
typedef LIST_HEAD(vmem_freelist, bt) vmem_freelist_t;
typedef LIST_HEAD(vmem_hashlist, bt) vmem_hashlist_t;

/*! \brief vmem structure
 *
 * Field markings and the corresponding locks:
 *  (a) vm_lock
 *  (@) vmem_list_lock
 *  (!) read-only access, do not modify!
 */
typedef struct vmem {
  LIST_ENTRY(vmem) vm_link; /* (@) link for vmem_list */
  mtx_t vm_lock;            /* vmem lock */
  size_t vm_size;           /* (a) total size of all added spans */
  size_t vm_inuse;          /* (a) total size of all allocated segments */
  size_t vm_quantum;    /* (!) alignment & the smallest unit of allocation */
  int vm_quantum_shift; /* (!) log2 of vm_quantum */
  char vm_name[VMEM_NAME_MAX]; /* (!) name of vmem instance */
  vmem_seglist_t vm_seglist;   /* (a) list of all segments */
  /* (a) table of lists of free segments */
  vmem_freelist_t vm_freelist[VMEM_MAXORDER];
  /* (a) hashtable of lists of allocated segments */
  vmem_hashlist_t vm_hashlist[VMEM_MAXHASH];
} vmem_t;

/*
 * The following interface is a simplified version of NetBSD's vmem interface.
 * For detailed documentation, please refer to VMEM(9), e.g. here:
 * https://netbsd.gw.com/cgi-bin/man-cgi?vmem+9+NetBSD-current
 */

/*! \brief Called during kernel initialization. */
void init_vmem(void);

/*! \brief Initialized a vmem arena.
 * You need to specify quantum, the smallest unit of allocation. */
void vmem_init(vmem_t *vm, const char *name, vmem_size_t quantum);

/*! \brief Allocates and initializes a vmem arena. */
vmem_t *vmem_create(const char *name, vmem_size_t quantum);

/*! \brief Obtain the size of the segment starting at `addr`. */
vmem_size_t vmem_size(vmem_t *vm, vmem_addr_t addr);

/*! \brief Add a new address span to the arena. */
int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size,
             kmem_flags_t flags);

/*! \brief Allocate an address segment from the arena. */
int vmem_alloc(vmem_t *vm, vmem_size_t size, vmem_addr_t *addrp,
               kmem_flags_t flags);

/*! \brief Free segment previously allocated by vmem_alloc(). */
void vmem_free(vmem_t *vm, vmem_addr_t addr);

/*! \brief Destroy existing vmem arena. */
void vmem_destroy(vmem_t *vm);

#endif /* _SYS_VMEM_H_ */
