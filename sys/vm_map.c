#define KL_LOG KL_VM
#include <klog.h>
#include <stdc.h>
#include <malloc.h>
#include <pmap.h>
#include <sync.h>
#include <thread.h>
#include <vm.h>
#include <vm_pager.h>
#include <vm_object.h>
#include <vm_map.h>
#include <errno.h>
#include <proc.h>
#include <mips/mips.h>
#include <pcpu.h>
#include <sysinit.h>

static MALLOC_DEFINE(M_VMMAP, "vm-map", 1, 2);

static vm_map_t kspace;

void vm_map_activate(vm_map_t *map) {
  critical_enter();
  PCPU_SET(uspace, map);
  pmap_activate(map ? map->pmap : NULL);
  critical_leave();
}

vm_map_t *get_user_vm_map() {
  return PCPU_GET(uspace);
}

vm_map_t *get_kernel_vm_map() {
  return &kspace;
}

static bool in_range(vm_map_t *map, vm_addr_t addr) {
  /* No need to enter RWlock, the pmap field is const. */
  return map && map->pmap->start <= addr && addr < map->pmap->end;
}

vm_map_t *get_active_vm_map_by_addr(vm_addr_t addr) {
  if (in_range(get_user_vm_map(), addr))
    return get_user_vm_map();
  if (in_range(get_kernel_vm_map(), addr))
    return get_kernel_vm_map();
  return NULL;
}

static inline int vm_map_entry_cmp(vm_map_entry_t *a, vm_map_entry_t *b) {
  if (a->start < b->start)
    return -1;
  return a->start - b->start;
}

SPLAY_PROTOTYPE(vm_map_tree, vm_map_entry, map_tree, vm_map_entry_cmp);
SPLAY_GENERATE(vm_map_tree, vm_map_entry, map_tree, vm_map_entry_cmp);

static void vm_map_setup(vm_map_t *map) {
  TAILQ_INIT(&map->list);
  SPLAY_INIT(&map->tree);
  rw_init(&map->rwlock, "vm map rwlock", 1);
}

void vm_map_init() {
  vm_map_setup(&kspace);
  *((pmap_t **)(&kspace.pmap)) = get_kernel_pmap();
}

vm_map_t *vm_map_new() {
  vm_map_t *map = kmalloc(M_VMMAP, sizeof(vm_map_t), M_ZERO);

  vm_map_setup(map);
  *((pmap_t **)&map->pmap) = pmap_new();
  return map;
}

static bool vm_map_insert_entry(vm_map_t *vm_map, vm_map_entry_t *entry) {
  rw_assert(&vm_map->rwlock, RW_WLOCKED);
  if (!SPLAY_INSERT(vm_map_tree, &vm_map->tree, entry)) {
    vm_map_entry_t *next = SPLAY_NEXT(vm_map_tree, &vm_map->tree, entry);
    if (next)
      TAILQ_INSERT_BEFORE(next, entry, map_list);
    else
      TAILQ_INSERT_TAIL(&vm_map->list, entry, map_list);
    vm_map->nentries++;
    return true;
  }
  return false;
}

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr) {
  vm_map_entry_t *etr_it;
  rw_scoped_enter(&vm_map->rwlock, RW_READER);
  TAILQ_FOREACH (etr_it, &vm_map->list, map_list)
    if (etr_it->start <= vaddr && vaddr < etr_it->end)
      return etr_it;
  return NULL;
}

static void vm_map_remove_entry(vm_map_t *vm_map, vm_map_entry_t *entry) {
  rw_assert(&vm_map->rwlock, RW_WLOCKED);
  vm_map->nentries--;
  vm_object_free(entry->object);
  TAILQ_REMOVE(&vm_map->list, entry, map_list);
  kfree(M_VMMAP, entry);
}

void vm_map_delete(vm_map_t *map) {
  rw_enter(&map->rwlock, RW_WRITER);
  while (map->nentries > 0)
    vm_map_remove_entry(map, TAILQ_FIRST(&map->list));
  rw_leave(&map->rwlock);

  kfree(M_VMMAP, map);
}

vm_map_entry_t *vm_map_add_entry(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                                 vm_prot_t prot) {
  assert(start >= map->pmap->start);
  assert(end <= map->pmap->end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  rw_scoped_enter(&map->rwlock, RW_WRITER);
#if 0
  assert(vm_map_find_entry(map, start) == NULL);
  assert(vm_map_find_entry(map, end) == NULL);
#endif

  vm_map_entry_t *entry = kmalloc(M_VMMAP, sizeof(vm_map_entry_t), M_ZERO);

  entry->start = start;
  entry->end = end;
  entry->prot = prot;

  vm_map_insert_entry(map, entry);

  return entry;
}

/* TODO: not implemented */
void vm_map_protect(vm_map_t *map, vm_addr_t start, vm_addr_t end,
                    vm_prot_t prot) {
}

int vm_map_findspace_nolock(vm_map_t *map, vm_addr_t start, size_t length,
                            vm_addr_t /*out*/ *addr) {
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(length, PAGESIZE));

  /* Bounds check */
  if (start < map->pmap->start)
    start = map->pmap->start;
  if (start + length > map->pmap->end)
    return -ENOMEM;

  /* Entire space free? */
  if (TAILQ_EMPTY(&map->list))
    goto found;

  /* Is enought space before the first entry in the map? */
  vm_map_entry_t *first = TAILQ_FIRST(&map->list);
  if (start + length <= first->start)
    goto found;

  /* Browse available gaps. */
  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &map->list, map_list) {
    vm_map_entry_t *next = TAILQ_NEXT(it, map_list);
    vm_addr_t gap_start = it->end;
    vm_addr_t gap_end = next ? next->start : map->pmap->end;

    /* Move start address forward if it points inside allocated space. */
    if (start < gap_start)
      start = gap_start;

    /* Will we fit inside this gap? */
    if (start + length <= gap_end)
      goto found;
  }

  /* Failed to find free space. */
  return -ENOMEM;

found:
  *addr = start;
  return 0;
}

int vm_map_findspace(vm_map_t *map, vm_addr_t start, size_t length,
                     vm_addr_t /*out*/ *addr) {
  rw_scoped_enter(&map->rwlock, RW_READER);
  return vm_map_findspace_nolock(map, start, length, addr);
}

int vm_map_resize(vm_map_t *map, vm_map_entry_t *entry, vm_addr_t new_end) {
  assert(is_aligned(new_end, PAGESIZE));
  rw_assert(&map->rwlock, RW_WLOCKED);

  /* TODO: As for now, we are unable to decrease the size of an entry, because
     it would require unmapping physical pages, which in turn should clean
     TLB. This is not implemented yet, and therefore shrinking an entry
     immediately leads to very confusing behavior, as the vm_map and TLB entries
     do not match. */
  assert(new_end >= entry->end);

  if (new_end > entry->end) {
    /* Expanding entry */
    vm_map_entry_t *next = TAILQ_NEXT(entry, map_list);
    vm_addr_t gap_end = next ? next->start : map->pmap->end;
    if (new_end > gap_end)
      return -ENOMEM;
  } else {
    /* Shrinking entry */
    if (new_end < entry->start)
      return -ENOMEM;
    /* TODO: Invalidate tlb? */
  }
  /* Note that neither tailq nor splay tree require updating. */
  entry->end = new_end;
  return 0;
}

void vm_map_dump(vm_map_t *map) {
  vm_map_entry_t *it;
  kprintf("[vm_map] Virtual memory map (%08lx - %08lx):\n", map->pmap->start,
          map->pmap->end);
  rw_scoped_enter(&map->rwlock, RW_READER);
  TAILQ_FOREACH (it, &map->list, map_list) {
    kprintf("[vm_map] * %08lx - %08lx [%c%c%c]\n", it->start, it->end,
            (it->prot & VM_PROT_READ) ? 'r' : '-',
            (it->prot & VM_PROT_WRITE) ? 'w' : '-',
            (it->prot & VM_PROT_EXEC) ? 'x' : '-');
    vm_map_object_dump(it->object);
  }
}

/* This entire function is a nasty hack, but we'll live with it until proper COW
   is implemented. */
vm_map_t *vm_map_clone(vm_map_t *map) {
  thread_t *td = thread_self();
  assert(td->td_proc);
  assert(td->td_proc->p_nthreads == 1);

  vm_map_t *orig_current_map = get_user_vm_map();
  vm_map_t *newmap = vm_map_new();

  rw_scoped_enter(&map->rwlock, RW_READER);

  /* Temporarily switch to the new map, so that we may write contents. */
  td->td_proc->p_uspace = newmap;
  vm_map_activate(newmap);

  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &map->list, map_list) {
    vm_map_entry_t *entry =
      vm_map_add_entry(newmap, it->start, it->end, it->prot);
    entry->object = default_pager->pgr_alloc();
    vm_page_t *page;
    TAILQ_FOREACH (page, &it->object->list, obj.list) {
      memcpy((char *)it->start + page->vm_offset,
             (char *)MIPS_PHYS_TO_KSEG0(page->paddr), page->size * PAGESIZE);
    }
  }

  /* Return to original vm map. */
  td->td_proc->p_uspace = orig_current_map;
  vm_map_activate(orig_current_map);

  return newmap;
}

int vm_page_fault(vm_map_t *map, vm_addr_t fault_addr, vm_prot_t fault_type) {
  vm_map_entry_t *entry;
  rw_scoped_enter(&map->rwlock, RW_READER);

  if (!(entry = vm_map_find_entry(map, fault_addr))) {
    klog("Tried to access unmapped memory region: 0x%08lx!", fault_addr);
    return -EFAULT;
  }

  if (entry->prot == VM_PROT_NONE) {
    klog("Cannot access to address: 0x%08lx", fault_addr);
    return -EACCES;
  }

  if (!(entry->prot & VM_PROT_WRITE) && (fault_type == VM_PROT_WRITE)) {
    klog("Cannot write to address: 0x%08lx", fault_addr);
    return -EACCES;
  }

  if (!(entry->prot & VM_PROT_READ) && (fault_type == VM_PROT_READ)) {
    klog("Cannot read from address: 0x%08lx", fault_addr);
    return -EACCES;
  }

  assert(entry->start <= fault_addr && fault_addr < entry->end);

  vm_object_t *obj = entry->object;

  assert(obj != NULL);

  vm_addr_t fault_page = fault_addr & -PAGESIZE;
  vm_addr_t offset = fault_page - entry->start;
  vm_page_t *frame = vm_object_find_page(entry->object, offset);

  if (!frame)
    frame = obj->pgr->pgr_fault(obj, fault_page, offset, fault_type);
  pmap_map(map->pmap, fault_page, fault_page + PAGESIZE, frame->paddr,
           entry->prot);

  return 0;
}
SYSINIT_ADD(vm_map, vm_map_init, DEPS("pmap", "vm_object"));
