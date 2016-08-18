#ifndef _VM_MAP_H_
#define _VM_MAP_H_

#include <queue.h>
#include <pmap.h>
#include <vm.h>

/* There has to be distinction between kernel and user vm_map,
 * That's because in current implementation page table is always located in
 * KSEG2, while user vm_map address range contains no KSEG2 */

typedef enum { KERNEL_VM_MAP = 1, USER_VM_MAP = 2 } vm_map_type_t;

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
  TAILQ_HEAD(, vm_map_entry) list;
  SPLAY_HEAD(vm_map_tree, vm_map_entry) tree;
  size_t nentries;

  vm_addr_t start;
  vm_addr_t end;

  pmap_t pmap;
} vm_map_t;

/* TODO we will need some functions to allocate address ranges,
 * since vm_map contains all information about address ranges, it's best idea
 * to embed allocation into this subsystem instead of building new one
 * on top of it. Function of similar interface would be basic:
 *
 * vm_map_entry_t* vm_map_allocate_space(vm_map_t* map, size_t length) */

void set_active_vm_map(vm_map_t *map);
vm_map_t *get_active_vm_map();

void vm_map_init();
vm_map_t *vm_map_new(vm_map_type_t t, asid_t asid);
void vm_map_delete(vm_map_t *vm_map);

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr);

void vm_map_protect(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                    vm_prot_t prot);
void vm_map_insert_object(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                          vm_object_t *object, vm_prot_t prot);

void vm_map_dump(vm_map_t *vm_map);
void vm_map_entry_dump(vm_map_entry_t *entry);

void vm_page_fault(vm_map_t *map, vm_addr_t fault_addr, vm_prot_t fault_type);

#endif /* _VM_MAP_H */
