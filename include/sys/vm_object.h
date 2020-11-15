#ifndef _SYS_VM_OBJECT_H_
#define _SYS_VM_OBJECT_H_

#include <sys/vm.h>
#include <sys/vm_pager.h>
#include <sys/mutex.h>
#include <sys/refcnt.h>

/* At the moment assume object is owned by only one vm_map */
typedef struct vm_object {
  mtx_t mtx;
  vm_pagelist_t list;
  vm_pagetree_t tree;
  size_t size;
  size_t npages;
  vm_pager_t *pager;
  refcnt_t ref_counter;
} vm_object_t;

vm_object_t *vm_object_alloc(vm_pgr_type_t type);
void vm_object_free(vm_object_t *obj);
bool vm_object_add_page(vm_object_t *obj, off_t offset, vm_page_t *pg);
void vm_object_remove_page(vm_object_t *obj, vm_page_t *pg);
void vm_object_remove_range(vm_object_t *obj, off_t offset, size_t length);
vm_page_t *vm_object_find_page(vm_object_t *obj, off_t offset);
vm_object_t *vm_object_clone(vm_object_t *obj);
void vm_map_object_dump(vm_object_t *obj);

#endif /* !_SYS_VM_OBJECT_H_ */
