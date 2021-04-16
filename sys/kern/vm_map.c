#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/vm_pager.h>
#include <sys/vm_object.h>
#include <sys/vm_map.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/pcpu.h>
#include <machine/vm_param.h>

struct vm_map_entry {
  TAILQ_ENTRY(vm_map_entry) link;
  vm_object_t *object;
  /* TODO(fz): add aref */
  vm_prot_t prot;
  vm_entry_flags_t flags;
  vaddr_t start;
  vaddr_t end;
};

struct vm_map {
  TAILQ_HEAD(vm_map_list, vm_map_entry) entries;
  size_t nentries;
  pmap_t *pmap;
  mtx_t mtx; /* Mutex guarding vm_map structure and all its entries. */
};

static POOL_DEFINE(P_VM_MAP, "vm_map", sizeof(vm_map_t));
static POOL_DEFINE(P_VM_MAPENT, "vm_map_entry", sizeof(vm_map_entry_t));

static vm_map_t *kspace = &(vm_map_t){};

void vm_map_activate(vm_map_t *map) {
  SCOPED_NO_PREEMPTION();

  PCPU_SET(uspace, map);
  pmap_activate(map ? map->pmap : NULL);
}

void vm_map_switch(thread_t *td) {
  proc_t *p = td->td_proc;
  if (p)
    vm_map_activate(p->p_uspace);
}

void vm_map_lock(vm_map_t *map) {
  mtx_lock(&map->mtx);
}

void vm_map_unlock(vm_map_t *map) {
  mtx_unlock(&map->mtx);
}

vm_map_t *vm_map_user(void) {
  return PCPU_GET(uspace);
}

vm_map_t *vm_map_kernel(void) {
  return kspace;
}

vaddr_t vm_map_start(vm_map_t *map) {
  return map->pmap == pmap_kernel() ? KERNEL_SPACE_BEGIN : USER_SPACE_BEGIN;
}

vaddr_t vm_map_end(vm_map_t *map) {
  return map->pmap == pmap_kernel() ? KERNEL_SPACE_END : USER_SPACE_END;
}

vaddr_t vm_map_entry_start(vm_map_entry_t *ent) {
  return ent->start;
}

vaddr_t vm_map_entry_end(vm_map_entry_t *ent) {
  return ent->end;
}

bool vm_map_address_p(vm_map_t *map, vaddr_t addr) {
  return map && vm_map_start(map) <= addr && addr < vm_map_end(map);
}

bool vm_map_contains_p(vm_map_t *map, vaddr_t start, vaddr_t end) {
  return map && vm_map_start(map) <= start && end <= vm_map_end(map);
}

vm_map_t *vm_map_lookup(vaddr_t addr) {
  if (vm_map_address_p(vm_map_user(), addr))
    return vm_map_user();
  if (vm_map_address_p(vm_map_kernel(), addr))
    return vm_map_kernel();
  return NULL;
}

static void vm_map_setup(vm_map_t *map) {
  TAILQ_INIT(&map->entries);
  mtx_init(&map->mtx, 0);
}

void init_vm_map(void) {
  vm_map_setup(kspace);
  kspace->pmap = pmap_kernel();
  vm_map_activate(kspace);
}

vm_map_t *vm_map_new(void) {
  vm_map_t *map = pool_alloc(P_VM_MAP, M_ZERO);
  vm_map_setup(map);
  map->pmap = pmap_new();
  return map;
}

vm_map_entry_t *vm_map_entry_alloc(vm_object_t *obj, vaddr_t start,
                                   vaddr_t end, vm_prot_t prot,
                                   vm_entry_flags_t flags) {
  assert(page_aligned_p(start) && page_aligned_p(end));

  vm_map_entry_t *ent = pool_alloc(P_VM_MAPENT, M_ZERO);
  ent->object = obj;
  ent->start = start;
  ent->end = end;
  ent->prot = prot;
  ent->flags = flags;
  return ent;
}

static void vm_map_entry_free(vm_map_entry_t *ent) {
  if (ent->object)
    vm_object_drop(ent->object);
  pool_free(P_VM_MAPENT, ent);
}

vm_map_entry_t *vm_map_find_entry(vm_map_t *map, vaddr_t vaddr) {
  assert(mtx_owned(&map->mtx));

  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &map->entries, link)
    if (it->start <= vaddr && vaddr < it->end)
      return it;
  return NULL;
}

static void vm_map_insert_after(vm_map_t *map, vm_map_entry_t *after,
                                vm_map_entry_t *ent) {
  assert(mtx_owned(&map->mtx));
  if (after)
    TAILQ_INSERT_AFTER(&map->entries, after, ent, link);
  else
    TAILQ_INSERT_HEAD(&map->entries, ent, link);
  map->nentries++;
}

void vm_map_entry_destroy(vm_map_t *map, vm_map_entry_t *ent) {
  assert(mtx_owned(&map->mtx));

  TAILQ_REMOVE(&map->entries, ent, link);
  map->nentries--;
  vm_map_entry_free(ent);
}

void vm_map_entry_destroy_range(vm_map_t *map, vm_map_entry_t *ent,
                                vaddr_t start, vaddr_t end) {
  assert(mtx_owned(&map->mtx));
  assert(start >= ent->start && end <= ent->end);

  pmap_remove(map->pmap, start, end);
  if (ent->start == start && ent->end == end) {
    vm_map_entry_destroy(map, ent);
    return;
  }

  size_t length = end - start;
  vm_object_remove_pages(ent->object, start - ent->start, length);

  if (ent->start == start) {
    ent->start = end;
  } else if (ent->end == end) {
    ent->end = start;
  } else { /* a hole inside the entry */
    vm_object_t *obj = vm_object_clone(ent->object);
    vm_object_remove_pages(obj, 0, start - ent->start);
    vm_object_remove_pages(ent->object, end - ent->start, ent->end - end);
    vm_map_entry_t *new_ent =
      vm_map_entry_alloc(obj, end, ent->end, ent->prot, ent->flags);
    ent->end = start;
    vm_map_insert_after(map, new_ent, ent);
  }
}

void vm_map_delete(vm_map_t *map) {
  pmap_delete(map->pmap);
  WITH_MTX_LOCK (&map->mtx) {
    vm_map_entry_t *ent, *next;
    TAILQ_FOREACH_SAFE (ent, &map->entries, link, next)
      vm_map_entry_destroy(map, ent);
  }
  pool_free(P_VM_MAP, map);
}

/* TODO: not implemented */
void vm_map_protect(vm_map_t *map, vaddr_t start, vaddr_t end, vm_prot_t prot) {
}

static int vm_map_findspace_nolock(vm_map_t *map, vaddr_t /*inout*/ *start_p,
                                   size_t length, vm_map_entry_t **after_p) {
  vaddr_t start = *start_p;

  assert(page_aligned_p(start) && page_aligned_p(length));

  /* Bounds check */
  start = max(start, vm_map_start(map));
  if (start + length > vm_map_end(map))
    return ENOMEM;

  if (after_p)
    *after_p = NULL;

  /* Entire space free? */
  if (TAILQ_EMPTY(&map->entries))
    goto found;

  /* Is enought space before the first entry in the map? */
  vm_map_entry_t *first = TAILQ_FIRST(&map->entries);
  if (start + length <= first->start)
    goto found;

  /* Browse available gaps. */
  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &map->entries, link) {
    vm_map_entry_t *next = TAILQ_NEXT(it, link);
    vaddr_t gap_start = it->end;
    vaddr_t gap_end = next ? next->start : vm_map_end(map);

    /* Move start address forward if it points inside allocated space. */
    if (start < gap_start)
      start = gap_start;

    /* Will we fit inside this gap? */
    if (start + length <= gap_end) {
      if (after_p)
        *after_p = it;
      goto found;
    }
  }

  /* Failed to find free space. */
  return ENOMEM;

found:
  *start_p = start;
  return 0;
}

int vm_map_findspace(vm_map_t *map, vaddr_t *start_p, size_t length) {
  SCOPED_MTX_LOCK(&map->mtx);
  return vm_map_findspace_nolock(map, start_p, length, NULL);
}

int vm_map_insert(vm_map_t *map, vm_map_entry_t *ent, vm_flags_t flags) {
  SCOPED_MTX_LOCK(&map->mtx);
  vm_map_entry_t *after;
  vaddr_t start = ent->start;
  size_t length = ent->end - ent->start;
  vm_entry_flags_t entry_flags = 0;

  int error = vm_map_findspace_nolock(map, &start, length, &after);
  if (error)
    return error;
  if ((flags & VM_FIXED) && (start != ent->start))
    return ENOMEM;

  assert((flags & (VM_SHARED | VM_PRIVATE)) != (VM_SHARED | VM_PRIVATE));

  entry_flags |= (flags & VM_SHARED) ? VM_ENT_SHARED : VM_ENT_PRIVATE;

  ent->start = start;
  ent->end = start + length;
  ent->flags = entry_flags;

  vm_map_insert_after(map, after, ent);
  return 0;
}

int vm_map_alloc_entry(vm_map_t *map, vaddr_t addr, size_t length,
                       vm_prot_t prot, vm_flags_t flags,
                       vm_map_entry_t **ent_p) {
  if (!(flags & VM_ANON)) {
    klog("Only anonymous memory mappings are supported!");
    return ENOTSUP;
  }

  if (!page_aligned_p(addr))
    return EINVAL;

  if (length == 0)
    return EINVAL;

  if (addr != 0 && !vm_map_contains_p(map, addr, addr + length))
    return EINVAL;

  /* Create object with a pager that supplies cleared pages on page fault. */
  vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
  vm_map_entry_t *ent =
    vm_map_entry_alloc(obj, addr, addr + length, prot, VM_ENT_SHARED);

  /* Given the hint try to insert the entry at given position or after it. */
  if (vm_map_insert(map, ent, flags)) {
    vm_map_entry_free(ent);
    return ENOMEM;
  }

  *ent_p = ent;
  return 0;
}

int vm_map_entry_resize(vm_map_t *map, vm_map_entry_t *ent, vaddr_t new_end) {
  assert(page_aligned_p(new_end));
  assert(new_end >= ent->start);
  SCOPED_MTX_LOCK(&map->mtx);

  if (new_end >= ent->end) {
    /* Expanding entry */
    vm_map_entry_t *next = TAILQ_NEXT(ent, link);
    vaddr_t gap_end = next ? next->start : vm_map_end(map);
    if (new_end > gap_end)
      return ENOMEM;
  } else {
    /* Shrinking entry */
    off_t offset = new_end - ent->start;
    size_t length = ent->end - new_end;
    pmap_remove(map->pmap, new_end, ent->end);
    vm_object_remove_pages(ent->object, offset, length);
    /* TODO there's no reference to pmap in page, so we have to do it here */
  }

  ent->end = new_end;

  if (ent->start == ent->end)
    vm_map_entry_destroy(map, ent);

  return 0;
}

void vm_map_dump(vm_map_t *map) {
  SCOPED_MTX_LOCK(&map->mtx);

  klog("Virtual memory map (%08lx - %08lx):", vm_map_start(map),
       vm_map_end(map));

  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &map->entries, link) {
    klog(" * %08lx - %08lx [%c%c%c]", it->start, it->end,
         (it->prot & VM_PROT_READ) ? 'r' : '-',
         (it->prot & VM_PROT_WRITE) ? 'w' : '-',
         (it->prot & VM_PROT_EXEC) ? 'x' : '-');
    vm_object_dump(it->object);
  }
}

vm_map_t *vm_map_clone(vm_map_t *map) {
  thread_t *td = thread_self();
  assert(td->td_proc);

  vm_map_t *new_map = vm_map_new();

  WITH_MTX_LOCK (&map->mtx) {
    vm_map_entry_t *it;
    TAILQ_FOREACH (it, &map->entries, link) {
      vm_object_t *obj;
      vm_map_entry_t *ent;

      if (it->flags & VM_ENT_SHARED) {
        vm_object_hold(it->object);
        obj = it->object;
      } else {
        /* vm_object_clone will clone the data from the vm_object_t
         * and will return the new object with ref_counter equal to one */
        obj = vm_object_clone(it->object);
      }
      ent = vm_map_entry_alloc(obj, it->start, it->end, it->prot, it->flags);
      TAILQ_INSERT_TAIL(&new_map->entries, ent, link);
      new_map->nentries++;
    }
  }

  return new_map;
}

int vm_page_fault(vm_map_t *map, vaddr_t fault_addr, vm_prot_t fault_type) {
  SCOPED_VM_MAP_LOCK(map);

  vm_map_entry_t *ent = vm_map_find_entry(map, fault_addr);

  if (!ent) {
    klog("Tried to access unmapped memory region: 0x%08lx!", fault_addr);
    return EFAULT;
  }

  if (ent->prot == VM_PROT_NONE) {
    klog("Cannot access to address: 0x%08lx", fault_addr);
    return EACCES;
  }

  if (!(ent->prot & VM_PROT_WRITE) && (fault_type == VM_PROT_WRITE)) {
    klog("Cannot write to address: 0x%08lx", fault_addr);
    return EACCES;
  }

  if (!(ent->prot & VM_PROT_READ) && (fault_type == VM_PROT_READ)) {
    klog("Cannot read from address: 0x%08lx", fault_addr);
    return EACCES;
  }

  assert(ent->start <= fault_addr && fault_addr < ent->end);

  vm_object_t *obj = ent->object;

  assert(obj != NULL);

  vaddr_t fault_page = fault_addr & -PAGESIZE;
  vaddr_t offset = fault_page - ent->start;
  vm_page_t *frame = vm_object_find_page(ent->object, offset);

  if (frame == NULL)
    frame = obj->vo_pager->pgr_fault(obj, offset);

  if (frame == NULL)
    return EFAULT;

  pmap_enter(map->pmap, fault_page, frame, ent->prot, 0);

  return 0;
}
