#ifndef _SYS_VM_OBJECT_H_
#define _SYS_VM_OBJECT_H_

#include <queue.h>
#include <pmap.h>
#include <vm.h>

/* At the moment assume object is owned by only one vm_map */
typedef struct vm_object {
  TAILQ_HEAD(, vm_page) list;
  RB_HEAD(vm_object_tree, vm_page) tree;
  size_t size;
  size_t npages;
  pager_t *pgr;
} vm_object_t;

/* TODO in future this function will need to be parametrized by type */
vm_object_t *vm_object_alloc(void);
void vm_object_free(vm_object_t *obj);
bool vm_object_add_page(vm_object_t *obj, vm_page_t *pg);
void vm_object_remove_page(vm_object_t *obj, vm_page_t *pg);
vm_page_t *vm_object_find_page(vm_object_t *obj, vm_addr_t offset);
void vm_map_object_dump(vm_object_t *obj);

#endif /* !_SYS_VM_OBJECT_H_ */
