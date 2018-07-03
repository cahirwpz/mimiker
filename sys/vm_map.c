#define KL_LOG KL_VM
#include <klog.h>
#include <stdc.h>
#include <pool.h>
#include <pmap.h>
#include <thread.h>
#include <vm.h>
#include <vm_pager.h>
#include <vm_object.h>
#include <vm_map.h>
#include <errno.h>
#include <proc.h>
#include <sched.h>
#include <pcpu.h>
#include <sysinit.h>

struct vm_map_entry {
  TAILQ_ENTRY(vm_map_entry) map_list;
  SPLAY_ENTRY(vm_map_entry) map_tree;
  vm_object_t *object;

  vm_prot_t prot;
  vm_addr_t start;
  vm_addr_t end;
};

struct vm_map {
  TAILQ_HEAD(vm_map_list, vm_map_entry) list;
  SPLAY_HEAD(vm_map_tree, vm_map_entry) tree;
  size_t nentries;
  pmap_t *const pmap;
  mtx_t mtx; /* Mutex guarding vm_map structure and all its entries. */
};

static POOL_DEFINE(P_VMMAP, "vm_map", sizeof(vm_map_t));
static POOL_DEFINE(P_VMENTRY, "vm_map_entry", sizeof(vm_map_entry_t));

static vm_map_t kspace;

void vm_map_activate(vm_map_t *map) {
  SCOPED_NO_PREEMPTION();

  PCPU_SET(uspace, map);
  pmap_activate(map ? map->pmap : NULL);
}

vm_map_t *get_user_vm_map(void) {
  return PCPU_GET(uspace);
}

vm_map_t *get_kernel_vm_map(void) {
  return &kspace;
}

void vm_map_entry_range(vm_map_entry_t *seg, vm_addr_t *start_p,
                        vm_addr_t *end_p) {
  *start_p = seg->start;
  *end_p = seg->end;
}

void vm_map_range(vm_map_t *map, vm_addr_t *start_p, vm_addr_t *end_p) {
  *start_p = map->pmap->start;
  *end_p = map->pmap->end;
}

bool vm_map_in_range(vm_map_t *map, vm_addr_t addr) {
  /* No need to enter RWlock, the pmap field is const. */
  return map && map->pmap->start <= addr && addr < map->pmap->end;
}

vm_map_t *get_active_vm_map_by_addr(vm_addr_t addr) {
  if (vm_map_in_range(get_user_vm_map(), addr))
    return get_user_vm_map();
  if (vm_map_in_range(get_kernel_vm_map(), addr))
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
  mtx_init(&map->mtx, MTX_DEF);
}

static void vm_map_init(void) {
  vm_map_setup(&kspace);
  *((pmap_t **)(&kspace.pmap)) = get_kernel_pmap();
}

vm_map_t *vm_map_new(void) {
  vm_map_t *map = pool_alloc(P_VMMAP, PF_ZERO);
  vm_map_setup(map);
  *((pmap_t **)&map->pmap) = pmap_new();
  return map;
}

static bool vm_map_insert_entry(vm_map_t *vm_map, vm_map_entry_t *entry) {
  assert(mtx_owned(&vm_map->mtx));
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

vm_map_entry_t *vm_map_find_entry(vm_map_t *map, vm_addr_t vaddr) {
  SCOPED_MTX_LOCK(&map->mtx);
  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &map->list, map_list)
    if (it->start <= vaddr && vaddr < it->end)
      return it;
  return NULL;
}

static void vm_map_remove_entry(vm_map_t *map, vm_map_entry_t *entry) {
  assert(mtx_owned(&map->mtx));
  map->nentries--;
  if (entry->object)
    vm_object_free(entry->object);
  TAILQ_REMOVE(&map->list, entry, map_list);
  pool_free(P_VMENTRY, entry);
}

void vm_map_delete(vm_map_t *map) {
  WITH_MTX_LOCK (&map->mtx) {
    while (map->nentries > 0)
      vm_map_remove_entry(map, TAILQ_FIRST(&map->list));
  }
  pmap_delete(map->pmap);
  pool_free(P_VMMAP, map);
}

static vm_map_entry_t *vm_map_insert_nolock(vm_map_t *map, vm_object_t *obj,
                                            vm_addr_t start, vm_addr_t end,
                                            vm_prot_t prot) {
  assert(mtx_owned(&map->mtx));
  assert(start >= map->pmap->start);
  assert(end <= map->pmap->end);
  assert(is_aligned(start, PAGESIZE));
  assert(is_aligned(end, PAGESIZE));

  vm_map_entry_t *entry = pool_alloc(P_VMENTRY, PF_ZERO);
  entry->object = obj;
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

static int vm_map_findspace_nolock(vm_map_t *map, vm_addr_t /*inout*/ *start_p,
                                   size_t length) {
  vm_addr_t start = *start_p;
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
  *start_p = start;
  return 0;
}

int vm_map_findspace(vm_map_t *map, vm_addr_t *start_p, size_t length) {
  SCOPED_MTX_LOCK(&map->mtx);
  return vm_map_findspace_nolock(map, start_p, length);
}

vm_map_entry_t *vm_map_insert(vm_map_t *map, vm_object_t *obj, vm_addr_t start,
                              vm_addr_t end, vm_prot_t prot) {
  SCOPED_MTX_LOCK(&map->mtx);
  return vm_map_insert_nolock(map, obj, start, end, prot);
}

vm_map_entry_t *vm_map_insert_anywhere(vm_map_t *map, vm_object_t *obj,
                                       vm_addr_t /* inout */ *start_p,
                                       size_t length, vm_prot_t prot) {
  SCOPED_MTX_LOCK(&map->mtx);
  if (vm_map_findspace_nolock(map, start_p, length) != 0)
    return NULL;
  vm_addr_t addr = *start_p;
  return vm_map_insert_nolock(map, obj, addr, addr + length, prot);
}

int vm_map_resize(vm_map_t *map, vm_map_entry_t *entry, vm_addr_t new_end) {
  assert(is_aligned(new_end, PAGESIZE));

  SCOPED_MTX_LOCK(&map->mtx);

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
  SCOPED_MTX_LOCK(&map->mtx);

  klog("Virtual memory map (%08lx - %08lx):", map->pmap->start, map->pmap->end);

  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &map->list, map_list) {
    klog(" * %08lx - %08lx [%c%c%c]", it->start, it->end,
         (it->prot & VM_PROT_READ) ? 'r' : '-',
         (it->prot & VM_PROT_WRITE) ? 'w' : '-',
         (it->prot & VM_PROT_EXEC) ? 'x' : '-');
    vm_map_object_dump(it->object);
  }
}

/* This entire function is a nasty hack,
 * but we'll live with it until proper COW is implemented. */
vm_map_t *vm_map_clone(vm_map_t *map) {
  thread_t *td = thread_self();
  assert(td->td_proc);

  vm_map_t *orig_current_map = get_user_vm_map();
  vm_map_t *newmap = vm_map_new();

  WITH_MTX_LOCK (&map->mtx) {
    /* Temporarily switch to the new map, so that we may write contents. */
    td->td_proc->p_uspace = newmap;
    vm_map_activate(newmap);

    vm_map_entry_t *it;
    TAILQ_FOREACH (it, &map->list, map_list) {
      vm_object_t *copy = vm_object_alloc(it->object->pager->pgr_type);
      (void)vm_map_insert_nolock(newmap, copy, it->start, it->end, it->prot);
      vm_page_t *page;
      TAILQ_FOREACH (page, &it->object->list, obj.list) {
        memcpy((char *)it->start + page->vm_offset,
               (char *)MIPS_PHYS_TO_KSEG0(page->paddr), page->size * PAGESIZE);
      }
    }
  }

  /* Return to original vm map. */
  td->td_proc->p_uspace = orig_current_map;
  vm_map_activate(orig_current_map);

  return newmap;
}

int vm_page_fault(vm_map_t *map, vm_addr_t fault_addr, vm_prot_t fault_type) {
  vm_map_entry_t *entry = NULL;

  entry = vm_map_find_entry(map, fault_addr);

  if (!entry) {
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
    frame = obj->pager->pgr_fault(obj, fault_page, offset, fault_type);
  pmap_map(map->pmap, fault_page, fault_page + PAGESIZE, frame->paddr,
           entry->prot);

  return 0;
}

SYSINIT_ADD(vm_map, vm_map_init, NODEPS);
