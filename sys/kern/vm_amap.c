#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/vm_map.h>
#include <sys/vm_amap.h>
#include <sys/vm_physmem.h>
#include <sys/refcnt.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/pcpu.h>
#include <machine/vm_param.h>
#include <machine/pte.h>

typedef struct vm_amap vm_amap_t;
typedef struct vm_aref vm_aref_t;

struct vm_amap {
  int slots;
  refcnt_t ref_cnt;
  vm_page_t **pg_list;
  bitstr_t *pg_bitmap;
};

static void vm_amap_free(vm_amap_t *amap);

static POOL_DEFINE(P_VM_AMAP_STRUCT, "vm_amap_struct", sizeof(vm_amap_t));

vm_amap_t *vm_amap_alloc(int slots) {
  vm_amap_t *amap = pool_alloc(P_VM_AMAP_STRUCT, M_WAITOK);
  if (!amap)
    return NULL;

  amap->pg_list = kmem_alloc(slots * sizeof(vm_page_t *), M_ZERO | M_WAITOK);
  if (!amap->pg_list) {
    pool_free(P_VM_AMAP_STRUCT, amap);
    return NULL;
  }

  amap->pg_bitmap = kmem_alloc(slots * sizeof(bitstr_t), M_ZERO | M_WAITOK);
  if (!amap->pg_bitmap) {
    kmem_free(amap->pg_list, slots * sizeof(vm_page_t *));
    pool_free(P_VM_AMAP_STRUCT, amap);
    return NULL;
  }

  amap->ref_cnt = 1;
  amap->slots = slots;
  return amap;
}

vm_amap_t *vm_amap_clone(vm_amap_t *amap) {
  if (!amap)
    return NULL;

  vm_amap_t *new = vm_amap_alloc(amap->slots);

  /* TODO: amap lock */
  for (int slot = 0; slot < amap->slots; slot++) {
    vm_page_t *pg = amap->pg_list[slot];
    if (pg) {
      vm_page_t *new_pg = vm_page_alloc(1);
      pmap_copy_page(pg, new_pg);
      new->pg_list[slot] = new_pg;
    }
  }
  return new;
}

void vm_amap_hold(vm_amap_t *amap) {
  refcnt_acquire(&amap->ref_cnt);
}

void vm_amap_drop(vm_amap_t *amap) {
  if (refcnt_release(&amap->ref_cnt)) {
    vm_amap_free(amap);
  }
}

vm_page_t *vm_amap_find_page(vm_aref_t aref, int offset) {
  /* TODO: amap lock */
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL);

  if (bit_test(amap->pg_bitmap, offset)) {
    return amap->pg_list[offset];
  }
  return NULL;
}

int vm_amap_add_page(vm_aref_t aref, vm_page_t *frame, int offset) {
  vm_amap_t *amap = aref.amap;
  assert(amap != NULL && frame != NULL);

  /* TODO: amap lock */
  if (bit_test(amap->pg_bitmap, offset)) {
    return EINVAL;
  }
  amap->pg_list[offset] = frame;
  bit_set(amap->pg_bitmap, offset);
  return 0;
}

static void _vm_amap_remove_pages(vm_amap_t *amap, int start, int nslots) {
  /* TODO: amap lock */
  for (int i = start; i < start + nslots; ++i) {
    vm_page_free(amap->pg_list[i]);
    bit_clear(amap->pg_bitmap, i);
  }
}

void vm_amap_remove_pages(vm_aref_t aref, int start, int nslots) {
  if (!aref.amap)
    return;
  _vm_amap_remove_pages(aref.amap, aref.offset + start, nslots);
}

static void vm_amap_free(vm_amap_t *amap) {
  _vm_amap_remove_pages(amap, 0, amap->slots);
  kmem_free(amap->pg_list, amap->slots * sizeof(vm_page_t *));
  kmem_free(amap->pg_bitmap, amap->slots * sizeof(bitstr_t));
  pool_free(P_VM_AMAP_STRUCT, amap);
}
