#ifndef _SYS_VM_OBJECT_H_
#define _SYS_VM_OBJECT_H_

#include <queue.h>
#include <tree.h>
#include <vm.h>
#include <vm_pager.h>

/* At the moment assume object is owned by only one vm_map */
typedef struct vm_object {
  TAILQ_HEAD(, vm_page) list;
  RB_HEAD(vm_object_tree, vm_page) tree;
  size_t size;
  size_t npages;
  vm_pager_t *pager;
} vm_object_t;

vm_object_t *vm_object_alloc(vm_pgr_type_t type);
void vm_object_free(vm_object_t *obj);
bool vm_object_add_page(vm_object_t *obj, vm_page_t *pg);
void vm_object_remove_page(vm_object_t *obj, vm_page_t *pg);
vm_page_t *vm_object_find_page(vm_object_t *obj, vm_addr_t offset);
vm_object_t *vm_map_object_clone(vm_object_t *obj);
void vm_map_object_dump(vm_object_t *obj);

#endif /* !_SYS_VM_OBJECT_H_ */
