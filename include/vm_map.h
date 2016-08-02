#ifndef _VM_MAP_H_
#define _VM_MAP_H_

#include <queue.h>
#include <pmap.h>

#define VM_PROTECTED   0x1
#define VM_READ        0x2
#define VM_WRITE       0x4

/* There has to be distinction between kernel and user vm_map,
 * That's because in current implementation page table is always located in
 * KSEG2, while user vm_map address range contains no KSEG2 */

typedef enum {
  KERNEL_VM_MAP = 1,
  USER_VM_MAP   = 2
} vm_map_type_t;

typedef struct vm_map_entry vm_map_entry_t;

typedef void (* page_fault_handler_t)(vm_map_entry_t *entry, 
                                      vm_addr_t fault_addr);

/* At the moment assume object is owned by only one vm_map */
typedef struct vm_object {
  TAILQ_HEAD(,vm_page) list;
  RB_HEAD(vm_object_tree, vm_page) tree;
  size_t size;
  size_t npages;

  page_fault_handler_t handler;
} vm_object_t;

struct vm_map_entry {
  TAILQ_ENTRY(vm_map_entry) map_list;
  SPLAY_ENTRY(vm_map_entry) map_tree; 
  vm_object_t *object;

  uint32_t flags;
  vm_addr_t start;
  vm_addr_t end;
};

typedef struct vm_map {
  TAILQ_HEAD(,vm_map_entry) list;
  SPLAY_HEAD(vm_map_tree, vm_map_entry) tree;
  size_t nentries;

  vm_addr_t start;
  vm_addr_t end;

  pmap_t pmap;
} vm_map_t;

/*  vm_map associated functions */
void vm_map_init();
vm_map_t* vm_map_new(vm_map_type_t t, asid_t asid);
void vm_map_delete(vm_map_t* vm_map);
vm_map_entry_t* vm_map_add_entry(vm_map_t* vm_map, uint32_t flags,
                                 size_t length, size_t alignment);
void vm_map_remove_entry(vm_map_t *vm_map, vm_map_entry_t *entry);
vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr);
void vm_map_dump(vm_map_t* vm_map);
void vm_map_entry_dump(vm_map_entry_t *entry);

void set_active_vm_map(vm_map_t* map);
vm_map_t* get_active_vm_map();

/*  vm_map_entry associated functions */
void vm_map_entry_map(vm_map_t *map, vm_map_entry_t *entry, 
                      vm_addr_t start, vm_addr_t end, uint8_t flags);
void vm_map_entry_unmap(vm_map_t *map, vm_map_entry_t *entry,
                        vm_addr_t start, vm_addr_t end);
void vm_map_entry_protect(vm_map_t *map, vm_map_entry_t *entry,
                          vm_addr_t start, vm_addr_t end, uint8_t flags);

/*  vm_object associated functions */
void vm_object_add_page(vm_object_t *obj, vm_page_t *pg);
void vm_object_remove_page(vm_object_t *obj, vm_page_t *pg);

#endif /* _VM_MAP_H */

