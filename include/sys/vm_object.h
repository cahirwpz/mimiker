#ifndef _SYS_VM_OBJECT_H_
#define _SYS_VM_OBJECT_H_

#include <sys/vm.h>
#include <sys/vm_pager.h>
#include <sys/mutex.h>
#include <sys/refcnt.h>
#include <sys/vm_map.h>
#include <sys/rwlock.h>

/*! \brief Virtual memory object
 *
 * Field marking and corresponding locks:
 * (a) atomic
 * (@) vm_object::mtx
 */

typedef struct vm_object {
  rwlock_t mtx;
  vm_pagelist_t list;   /* (@) List of pages */
  size_t npages;        /* (@) Number of pages */
  vm_pager_t *pager;    /* Pager type and page fault function for object */
  refcnt_t ref_counter; /* (a) How many objects refer to this object? */
  vm_object_t *shadow_object; /* (@) Pointer to backing object */
} vm_object_t;

vm_object_t *vm_object_alloc(vm_pgr_type_t type);
void vm_object_free(vm_object_t *obj);
void vm_object_add_page(vm_object_t *obj, off_t offset, vm_page_t *pg);
void vm_object_remove_page(vm_object_t *obj, vm_page_t *pg);
void vm_object_remove_range(vm_object_t *obj, off_t offset, size_t length);
vm_page_t *vm_object_find_page(vm_object_t *obj, off_t offset);
vm_object_t *vm_object_clone(vm_object_t *obj);
void vm_map_object_dump(vm_object_t *obj);
void vm_object_set_readonly(vm_object_t *obj);
void vm_object_increase_pages_references(vm_object_t *obj);
#endif /* !_SYS_VM_OBJECT_H_ */
