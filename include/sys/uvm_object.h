#ifndef _SYS_VM_OBJECT_H_
#define _SYS_VM_OBJECT_H_

#include <sys/vm.h>
#include <sys/vm_pager.h>
#include <sys/mutex.h>
#include <sys/refcnt.h>

/*! \brief Virtual memory object
 *
 * Field marking and corresponding locks:
 * (a) atomic
 * (@) vm_object::uo_lock
 */

typedef struct uvm_object {
  mtx_t uo_lock;
  vm_pagelist_t uo_pages; /* (@) List of pages */
  size_t uo_npages;       /* (@) Number of pages */
  vm_pager_t *uo_pager;   /* Pager type and page fault function for object */
  refcnt_t uo_refs;       /* (a) How many objects refer to this object? */
} uvm_object_t;

uvm_object_t *uvm_object_alloc(vm_pgr_type_t type);
void uvm_object_hold(uvm_object_t *obj);
void uvm_object_drop(uvm_object_t *obj);
void uvm_object_add_page(uvm_object_t *obj, vm_offset_t off, vm_page_t *pg);
void uvm_object_remove_pages(uvm_object_t *obj, vm_offset_t off, size_t len);
vm_page_t *uvm_object_find_page(uvm_object_t *obj, vm_offset_t off);
uvm_object_t *uvm_object_clone(uvm_object_t *obj);
void uvm_object_dump(uvm_object_t *obj);

#endif /* !_SYS_VM_OBJECT_H_ */
