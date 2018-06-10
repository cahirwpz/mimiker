#ifndef _SYS_VM_MAP_H_
#define _SYS_VM_MAP_H_

#include <queue.h>
#include <pmap.h>
#include <vm.h>
#include <mutex.h>

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
  pmap_t *const pmap;
  mtx_t mtx; /* Mutex guarding vm_map structure and all its entries. */
} vm_map_t;

#define WITH_VM_MAP_LOCK(ptr) WITH_MTX_LOCK (&(ptr)->mtx)
#define SCOPED_VM_MAP_LOCK(ptr) SCOPED_MTX_LOCK(&(ptr)->mtx)

/* TODO we will need some functions to allocate address ranges,
 * since vm_map contains all information about address ranges, it's best idea
 * to embed allocation into this subsystem instead of building new one
 * on top of it. Function of similar interface would be basic:
 *
 * vm_map_entry_t* vm_map_allocate_space(vm_map_t* map, size_t length) */

void vm_map_activate(vm_map_t *map);
vm_map_t *get_user_vm_map(void);
vm_map_t *get_kernel_vm_map(void);
vm_map_t *get_active_vm_map_by_addr(vm_addr_t addr);

vm_map_t *vm_map_new(void);
void vm_map_delete(vm_map_t *vm_map);

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr);

void vm_map_protect(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                    vm_prot_t prot);
vm_map_entry_t *vm_map_add_entry_nolock(vm_map_t *map, vm_addr_t start,
                                        vm_addr_t end, vm_prot_t prot);

vm_map_entry_t *vm_map_add_entry(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                                 vm_prot_t prot);

/* Looks up a gap of @length size in @map. The search starts at @start address.
 * On success, returns 0 and sets *addr. @start and @length arguments must be
 * page-aligned. */
int vm_map_findspace(vm_map_t *map, vm_addr_t start, size_t length,
                     vm_addr_t /*out*/ *addr);
int vm_map_findspace_nolock(vm_map_t *map, vm_addr_t start, size_t length,
                            vm_addr_t /*out*/ *addr);

/* Tries to resize an entry, by moving its end if there
   are no other mappings in the way. On success, returns 0. */
int vm_map_resize(vm_map_t *map, vm_map_entry_t *entry, vm_addr_t new_end);

void vm_map_dump(vm_map_t *vm_map);

vm_map_t *vm_map_clone(vm_map_t *map);

int vm_page_fault(vm_map_t *map, vm_addr_t fault_addr, vm_prot_t fault_type);

#endif /* !_SYS_VM_MAP_H_ */
