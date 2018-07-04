#ifndef _SYS_VM_MAP_H_
#define _SYS_VM_MAP_H_

#include <queue.h>
#include <pmap.h>
#include <vm.h>
#include <mutex.h>

void vm_map_activate(vm_map_t *map);

vm_map_t *get_user_vm_map(void);
vm_map_t *get_kernel_vm_map(void);
vm_map_t *get_active_vm_map_by_addr(vm_addr_t addr);

vm_map_t *vm_map_new(void);
void vm_map_delete(vm_map_t *vm_map);

vm_map_entry_t *vm_map_entry_alloc(vm_object_t *obj, vm_addr_t start,
                                   vm_addr_t end, vm_prot_t prot);
void vm_map_entry_free(vm_map_entry_t *entry);

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr);

void vm_map_protect(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                    vm_prot_t prot);

/*! \brief Insert given \a entry into the \a map. */
int vm_map_insert(vm_map_t *map, vm_map_entry_t *entry, vm_flags_t flags);

/*! \brief Can address \a addr be mapped by this \a map? */
bool vm_map_in_range(vm_map_t *map, vm_addr_t addr);

/*! \brief Reports range of addresses that are handled by the map. */
void vm_map_range(vm_map_t *map, vm_addr_t *start_p, vm_addr_t *end_p);

/*! \brief Reports range of addresses that are handled by the entry. */
void vm_map_entry_range(vm_map_entry_t *seg, vm_addr_t *start_p,
                        vm_addr_t *end_p);

/*! \brief Looks up a gap of \a length size in \a map.
 *
 * The search starts at an address read from \a start_p location.
 * Both start address and \a length arguments must be page-aligned.
 *
 * \returns 0 on success, sets *start_p as well
 */
int vm_map_findspace(vm_map_t *map, vm_addr_t /*inout*/ *start_p,
                     size_t length);

/* Tries to resize an entry, by moving its end if there
   are no other mappings in the way. On success, returns 0. */
int vm_map_resize(vm_map_t *map, vm_map_entry_t *entry, vm_addr_t new_end);

void vm_map_dump(vm_map_t *vm_map);

vm_map_t *vm_map_clone(vm_map_t *map);

int vm_page_fault(vm_map_t *map, vm_addr_t fault_addr, vm_prot_t fault_type);

#endif /* !_SYS_VM_MAP_H_ */
