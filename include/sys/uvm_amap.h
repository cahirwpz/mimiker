#ifndef _SYS_UVM_AMAP_H_
#define _SYS_UVM_AMAP_H_

#ifndef _KERNEL
#error "Do not use this header file outside kernel code!"
#endif

#include <sys/mutex.h>

typedef struct uvm_anon uvm_anon_t;
typedef struct uvm_aref uvm_aref_t;
typedef struct uvm_amap uvm_amap_t;

struct uvm_aref {
  int ar_pageoff;      /* page offset into amap we start */
  uvm_amap_t *ar_amap; /* pointer to amap */
};

/*
 * The upper layer of UVM’s two-layer mapping scheme.
 * A uvm_amap describes an area of anonymous memory.
 *
 * Field markings and the corresponding locks:
 *  (@) uvm_amap::am_lock
 */
struct uvm_amap {
  mtx_t am_lock;
  int am_ref;     /* (@) reference counter */
  int am_nslot;   /* (@) number of allocated slots */
  int am_nused;   /* (@) number of used slots */
  int *am_slot;   /* (@) slots of used anons - refers to am_bckptr */
  int *am_bckptr; /* (@) stack of used anons - refers to am_anon & am_slots */
  uvm_anon_t **am_anon; /* (@) anons in that map */
};

/*
 * amap interface
 */

/* Allocate a new amap. */
uvm_amap_t *uvm_amap_alloc(void);
/* Acquire amap->am_lock. */
void uvm_amap_lock(uvm_amap_t *amap);
/* Release amap->am_lock. */
void uvm_amap_unlock(uvm_amap_t *amap);
/* Increase reference counter. */
void uvm_amap_hold(uvm_amap_t *amap);
/* Decrement reference counter and destroy amap if it has reached 0. */
void uvm_amap_drop(uvm_amap_t *amap);
/* Lookup an anon at offset in amap. */
uvm_anon_t *uvm_amap_lookup(uvm_aref_t *aref, vaddr_t offset);
/* Add an annon at offset in amap. */
void uvm_amap_add(uvm_aref_t *aref, uvm_anon_t *anon, vaddr_t offset);
/* Remove an annon from an amap. */
void uvm_amap_remove(uvm_aref_t *aref, vaddr_t offset);

/* TODO: ppref */

/* TODO: aref interface */

#endif /* !_SYS_UVM_AMAP_H_ */
