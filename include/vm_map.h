#ifndef _VM_MAP_H_
#define _VM_MAP_H_

#include <queue.h>
#include <pmap.h>
#include <vm.h>

typedef struct vm_map_entry vm_map_entry_t;

struct vm_map_entry {
  TAILQ_ENTRY(vm_map_entry) map_list;
  SPLAY_ENTRY(vm_map_entry) map_tree;
  vm_object_t *object;

  uint32_t prot;
  vm_addr_t start;
  vm_addr_t end;
};

typedef struct vm_map {
  TAILQ_HEAD(vm_map_list, vm_map_entry) list;
  SPLAY_HEAD(vm_map_tree, vm_map_entry) tree;
  size_t nentries;
  pmap_t *pmap;
} vm_map_t;

/* TODO we will need some functions to allocate address ranges,
 * since vm_map contains all information about address ranges, it's best idea
 * to embed allocation into this subsystem instead of building new one
 * on top of it. Function of similar interface would be basic:
 *
 * vm_map_entry_t* vm_map_allocate_space(vm_map_t* map, size_t length) */

vm_map_t *vm_map_activate(vm_map_t *map);
vm_map_t *get_user_vm_map();
vm_map_t *get_kernel_vm_map();
vm_map_t *get_active_vm_map_by_addr(vm_addr_t addr);

void vm_map_init();
vm_map_t *vm_map_new();
void vm_map_delete(vm_map_t *vm_map);

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr);

void vm_map_protect(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                    vm_prot_t prot);
vm_map_entry_t *vm_map_add_entry(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                                 vm_prot_t prot);

int vm_map_findspace(vm_map_t *map, vm_addr_t start, size_t length,
                     vm_addr_t /*out*/ *addr);

void vm_map_dump(vm_map_t *vm_map);

void vm_page_fault(vm_map_t *map, vm_addr_t fault_addr, vm_prot_t fault_type);

#endif /* _VM_MAP_H */
