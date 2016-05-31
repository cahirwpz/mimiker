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

#define KERNEL_VM_MAP 1     /* 0xc0000000, 0xe0000000 */
#define USER_VM_MAP   2     /* 0x00000000, 0x80000000 */

#define KERNEL_VM_MAP_START 0xc0000000
#define KERNEL_VM_MAP_END 0xe0000000

#define USER_VM_MAP_START 0x00000000
#define USER_VM_MAP_END 0x80000000

#define PAGE_ALIGNMENT (1 << 12)
extern vm_map_t *cur_vm_map;

typedef uint32_t vm_map_type_t;

struct vm_object /* At the moment assume object is owned by only one vm_space */
{
  TAILQ_HEAD(,vm_page) list;
  RB_HEAD(vm_object_tree, vm_page) tree;

  size_t size;
  size_t refcount;
};

struct vm_map_entry
{
  TAILQ_ENTRY(vm_map_entry) map_list;
  SPLAY_ENTRY(vm_map_entry) map_tree; 
  vm_object_t *object;

  uint32_t flags;
  vm_addr_t start;
  vm_addr_t end;
};

struct vm_map
{
    TAILQ_HEAD(,vm_map_entry) list;
    SPLAY_HEAD(vm_map_tree, vm_map_entry) tree;
    size_t nentries;

    vm_addr_t start;
    vm_addr_t end;
    asid_t asid;

    pmap_t pmap;
};

void vm_map_init();
vm_map_t* vm_map_new(vm_map_type_t t, asid_t asid);
void vm_map_delete(vm_map_t* vm_map);
vm_map_entry_t* vm_map_add_entry(vm_map_t* vm_map, uint32_t flags, size_t length, size_t alignment);
void vm_map_remove_entry(vm_map_t *vm_map, vm_map_entry_t *entry);
vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr);
void vm_map_print(vm_map_t* vm_map);

void vm_map_entry_attach_pages(vm_map_entry_t *entry, vm_addr_t start, vm_addr_t end);
void vm_object_print(vm_object_t *obj);
void vm_object_insert_page(vm_object_t *obj, vm_page_t *page);

void set_vm_map(vm_map_t *vm_map);

#endif /* _VM_MAP_H */
