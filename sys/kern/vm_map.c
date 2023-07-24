#define KL_LOG KL_VM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/mutex.h>
#include <sys/libkern.h>
#include <sys/pool.h>
#include <sys/pmap.h>
#include <sys/vm_physmem.h>
#include <sys/vm_map.h>
#include <sys/vm_amap.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/pcpu.h>
#include <machine/vm_param.h>

struct vm_map_entry {
  TAILQ_ENTRY(vm_map_entry) link;
  vm_aref_t aref;
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

void vm_map_activate(vm_map_t *map) {
  SCOPED_NO_PREEMPTION();

  PCPU_SET(uspace, map);
  pmap_activate(map ? map->pmap : NULL);
}

void vm_map_switch(thread_t *td) {
  proc_t *p = td->td_proc;
  vm_map_activate(p ? p->p_uspace : NULL);
}

void vm_map_lock(vm_map_t *map) {
  mtx_lock(&map->mtx);
}

void vm_map_unlock(vm_map_t *map) {
  mtx_unlock(&map->mtx);
}

vm_map_t *vm_map_user(void) {
  vm_map_t *map = PCPU_GET(uspace);
  assert(map);
  return map;
}

vaddr_t vm_map_entry_start(vm_map_entry_t *ent) {
  return ent->start;
}

vaddr_t vm_map_entry_end(vm_map_entry_t *ent) {
  return ent->end;
}

inline vm_map_entry_t *vm_map_entry_next(vm_map_entry_t *ent) {
  return TAILQ_NEXT(ent, link);
}

static bool userspace_p(vaddr_t start, vaddr_t end) {
  return USER_SPACE_BEGIN <= start && end <= USER_SPACE_END;
}

static void vm_map_setup(vm_map_t *map) {
  TAILQ_INIT(&map->entries);
  mtx_init(&map->mtx, 0);
}

vm_map_t *vm_map_new(void) {
  vm_map_t *map = pool_alloc(P_VM_MAP, M_ZERO);
  vm_map_setup(map);
  map->pmap = pmap_new();
  return map;
}

vm_map_entry_t *vm_map_entry_alloc(vaddr_t start, vaddr_t end, vm_prot_t prot,
                                   vm_entry_flags_t flags) {
  assert(page_aligned_p(start) && page_aligned_p(end));

  vm_map_entry_t *ent = pool_alloc(P_VM_MAPENT, M_ZERO);

  ent->aref = (vm_aref_t){.offset = 0, .amap = NULL};
  ent->start = start;
  ent->end = end;
  ent->prot = prot;
  ent->flags = flags;
  return ent;
}

static void vm_map_entry_free(vm_map_entry_t *ent) {
  if (ent->aref.amap)
    vm_amap_drop(ent->aref.amap);
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

static void vm_map_entry_destroy(vm_map_t *map, vm_map_entry_t *ent) {
  assert(mtx_owned(&map->mtx));

  TAILQ_REMOVE(&map->entries, ent, link);
  map->nentries--;
  vm_map_entry_free(ent);
}

/* XXX: notice that amap is here copied but we don't increase ref_cnt. If it is
 * needed it must be done after calling this function.
 */
static inline vm_map_entry_t *vm_map_entry_copy(vm_map_entry_t *src) {
  vm_map_entry_t *ent =
    vm_map_entry_alloc(src->start, src->end, src->prot, src->flags);
  ent->aref = src->aref;
  return ent;
}

static inline bool range_intersects_map_entry(vm_map_entry_t *ent,
                                              vaddr_t start, vaddr_t end) {
  return vm_map_entry_end(ent) > start && vm_map_entry_start(ent) < end;
}

static inline size_t vaddr_to_slot(vaddr_t addr) {
  return addr / PAGESIZE;
}

/* Split vm_map_entry into two not empty entries. (Smallest possible entry is
 * entry with one page thus splitat must be page aligned.)
 *
 * Returns entry which is after base entry. */
static vm_map_entry_t *vm_map_entry_split(vm_map_t *map, vm_map_entry_t *ent,
                                          vaddr_t splitat) {
  assert(mtx_owned(&map->mtx));
  assert(page_aligned_p(splitat));
  assert(ent->start < splitat && splitat < ent->end);

  vm_map_entry_t *new_ent = vm_map_entry_copy(ent);

  /* Split amap if it is present. */
  vm_aref_t old = ent->aref;
  if (old.amap) {
    vm_amap_hold(old.amap);
    size_t offset = vaddr_to_slot(splitat - ent->start);
    new_ent->aref =
      (vm_aref_t){.offset = old.offset + offset, .amap = old.amap};
  }

  /* clip both entries */
  ent->end = splitat;
  new_ent->start = splitat;

  vm_map_insert_after(map, ent, new_ent);
  return new_ent;
}

static int vm_map_destroy_range_nolock(vm_map_t *map, vaddr_t start,
                                       vaddr_t end) {
  assert(mtx_owned(&map->mtx));

  /* Find first entry affected by unmapping memory. */
  vm_map_entry_t *ent = vm_map_find_entry(map, start);
  if (!ent)
    return 0;

  pmap_remove(map->pmap, start, end);

  while (range_intersects_map_entry(ent, start, end)) {
    vaddr_t rm_start = max(start, vm_map_entry_start(ent));
    vaddr_t rm_end = min(end, vm_map_entry_end(ent));

    /* Next entry that could be affected is right after current one.
     * Since we can delete it entirely, we have to take next entry now. */
    vm_map_entry_t *next = vm_map_entry_next(ent);

    vm_map_entry_t *del = ent;

    if (rm_start > ent->start) {
      /* entry we want to delete is after clipped entry */
      del = vm_map_entry_split(map, ent, rm_start);
    }

    if (rm_end < del->end) {
      /* entry which is after del is one we want to keep */
      vm_map_entry_split(map, del, rm_end);
    }

    vm_map_entry_destroy(map, del);

    if (!next)
      break;

    ent = next;
  }
  return 0;
}

int vm_map_destroy_range(vm_map_t *map, vaddr_t start, vaddr_t end) {
  SCOPED_VM_MAP_LOCK(map);
  return vm_map_destroy_range_nolock(map, start, end);
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

int vm_map_protect(vm_map_t *map, vaddr_t start, vaddr_t end, vm_prot_t prot) {
  SCOPED_MTX_LOCK(&map->mtx);

  vm_map_entry_t *ent = vm_map_find_entry(map, start);
  if (!ent)
    return ENOMEM;

  while (range_intersects_map_entry(ent, start, end)) {
    vaddr_t prot_start = max(start, vm_map_entry_start(ent));
    vaddr_t prot_end = min(end, vm_map_entry_end(ent));
    vm_map_entry_t *affected = ent;

    if (prot_start > ent->start) {
      /* entry we want to change is after clipped entry */
      affected = vm_map_entry_split(map, ent, prot_start);
    }

    if (prot_end < affected->end) {
      /* entry which is after affected is one we want to keep unchanged */
      vm_map_entry_split(map, affected, prot_end);
    }

    klog("change prot of %lx-%lx to %x", affected->start, affected->end, prot);

    pmap_protect(map->pmap, affected->start, affected->end, prot);
    affected->prot = prot;

    /* Everything done. */
    if (vm_map_entry_end(affected) == end)
      break;

    vm_map_entry_t *next = vm_map_entry_next(affected);

    /* If next entry doesn't exist or there is a gap return error. Tried to
     * change protection of unmapped memory. */
    if (!next || vm_map_entry_end(affected) != vm_map_entry_start(next))
      return ENOMEM;

    ent = next;
  }
  return 0;
}

static int vm_map_findspace_nolock(vm_map_t *map, vaddr_t /*inout*/ *start_p,
                                   size_t length, vm_map_entry_t **after_p) {
  vaddr_t start = *start_p;

  assert(page_aligned_p(start) && page_aligned_p(length));

  /* Bounds check */
  start = max(start, (vaddr_t)USER_SPACE_BEGIN);
  if (start + length > (vaddr_t)USER_SPACE_END)
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
    vm_map_entry_t *next = vm_map_entry_next(it);
    vaddr_t gap_start = it->end;
    vaddr_t gap_end = next ? next->start : USER_SPACE_END;

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

  int error;
  if ((flags & VM_FIXED) && !(flags & VM_EXCL)) {
    if ((error = vm_map_destroy_range_nolock(map, ent->start, ent->end)))
      return error;
  }

  if ((error = vm_map_findspace_nolock(map, &start, length, &after)))
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

  if (addr != 0 && !userspace_p(addr, addr + length))
    return EINVAL;

  /* Create entry without amap. */
  vm_map_entry_t *ent =
    vm_map_entry_alloc(addr, addr + length, prot, VM_ENT_SHARED);

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
    vm_map_entry_t *next = vm_map_entry_next(ent);
    vaddr_t gap_end = next ? next->start : USER_SPACE_END;
    if (new_end > gap_end)
      return ENOMEM;

    size_t new_slots = vaddr_to_slot(new_end - ent->start);
    if (ent->aref.amap && vm_amap_slots(ent->aref.amap) < new_slots)
      return ENOMEM;
  } else {
    /* Shrinking entry */

    /* There's no reference to pmap in page, so we have to do it here. */
    pmap_remove(map->pmap, new_end, ent->end);

    size_t offset = vaddr_to_slot(new_end - ent->start);
    size_t n_remove = vaddr_to_slot(ent->end - new_end);
    vm_amap_remove_pages(ent->aref, offset, n_remove);
  }

  ent->end = new_end;

  if (ent->start == ent->end)
    vm_map_entry_destroy(map, ent);

  return 0;
}

void vm_map_dump(vm_map_t *map) {
  SCOPED_MTX_LOCK(&map->mtx);

  klog("Virtual memory map (%08lx - %08lx):", USER_SPACE_BEGIN, USER_SPACE_END);

  vm_map_entry_t *it;
  TAILQ_FOREACH (it, &map->entries, link) {
    klog(" * %08lx - %08lx [%c%c%c]", it->start, it->end,
         (it->prot & VM_PROT_READ) ? 'r' : '-',
         (it->prot & VM_PROT_WRITE) ? 'w' : '-',
         (it->prot & VM_PROT_EXEC) ? 'x' : '-');
  }
}

static vm_map_entry_t *vm_map_entry_clone_shared(vm_map_t *map,
                                                 vm_map_entry_t *ent) {
  if (!ent->aref.amap) {
    /* We need to create amap, because we won't be able to share it if it
     * is not created now. */
    size_t slots = vaddr_to_slot(ent->end - ent->start);
    ent->aref.amap = vm_amap_alloc(slots);
  }

  vm_map_entry_t *new = vm_map_entry_copy(ent);
  vm_amap_hold(ent->aref.amap);
  new->aref = ent->aref;
  return new;
}

static vm_map_entry_t *vm_map_entry_clone_copy(vm_map_t *map,
                                               vm_map_entry_t *ent) {
  vm_map_entry_t *new = vm_map_entry_copy(ent);

  /* TODO(cow): There are few special cases but we don't need them now because
   * we don't support all the features available in original UVM system.
   *   - Share inheritance when Needs Copy flag is set
   *   - Amap is shared between 2 processes but must be copied due to
   *     copy inheritance of mapping
   *
   * -- Section 4.7 in "The Design and Implementation of UVM".
   */

  /* Just hold the amap if it is present and set flags. Copying will be done
   * during page fault. */
  if (ent->aref.amap)
    vm_amap_hold(ent->aref.amap);

  ent->flags |= VM_ENT_COW | VM_ENT_NEEDSCOPY;
  new->flags |= VM_ENT_COW | VM_ENT_NEEDSCOPY;

  pmap_protect(map->pmap, ent->start, ent->end, ent->prot & (~VM_PROT_WRITE));
  return new;
}

#define VM_ENT_INHERIT_MASK (VM_ENT_SHARED | VM_ENT_PRIVATE)

vm_map_t *vm_map_clone(vm_map_t *map) {
  thread_t *td = thread_self();
  assert(td->td_proc);

  vm_map_t *new_map = vm_map_new();

  WITH_MTX_LOCK (&map->mtx) {
    vm_map_entry_t *it, *new;
    TAILQ_FOREACH (it, &map->entries, link) {
      switch (it->flags & VM_ENT_INHERIT_MASK) {
        case VM_ENT_SHARED:
          new = vm_map_entry_clone_shared(map, it);
          break;
        case VM_ENT_PRIVATE:
          new = vm_map_entry_clone_copy(map, it);
          break;
        default:
          panic("Unrecognized vm_map_entry inheritance flag: %d",
                it->flags & VM_ENT_INHERIT_MASK);
      }

      /* Failed to create new entry. */
      if (!new) {
        vm_map_delete(new_map);
        return NULL;
      }

      TAILQ_INSERT_TAIL(&new_map->entries, new, link);
      new_map->nentries++;
    }
  }
  return new_map;
}

static int insert_anon(vm_map_entry_t *ent, size_t off, vm_anon_t **anonp,
                       vm_prot_t *ins_protp) {
  if (*anonp) {
    /* It's already present */
    *ins_protp = ent->prot;
    return 0;
  }

  vm_anon_t *new = vm_anon_alloc();
  if (!new)
    return ENOMEM;

  vm_anon_t *old = vm_amap_insert_anon(ent->aref, new, off);
  assert(old == NULL);

  *ins_protp = ent->prot;
  *anonp = new;
  return 0;
}

static int cow_page_fault(vm_map_t *map, vm_map_entry_t *ent, size_t off,
                          vm_anon_t **anonp, vm_prot_t *ins_protp,
                          vm_prot_t access) {
  /* We don't need to modify amap. We do the same as in non-cow case but we have
   * to limit protection of page that will be inserted. */
  if ((access & VM_PROT_WRITE) == 0) {
    int err = insert_anon(ent, off, anonp, ins_protp);
    *ins_protp = ent->prot & ~VM_PROT_WRITE;
    return err;
  }

  /* Copy amap to make it ready for inserting copied or new anons. */
  if (ent->flags & VM_ENT_NEEDSCOPY) {
    size_t amap_slots = vaddr_to_slot(ent->end - ent->start);
    vm_aref_t new_aref = vm_amap_needs_copy(ent->aref, amap_slots);
    if (!new_aref.amap)
      return ENOMEM;
    ent->flags &= ~VM_ENT_NEEDSCOPY;
    ent->aref = new_aref;
  }

  /* If we need to create a new anon and insert it into amap we can do that
   * using insert_anon procedure. (Now it will operate on new amap if it was
   * copied.) */
  if (*anonp == NULL)
    return insert_anon(ent, off, anonp, ins_protp);

  /* Current mapping will be replaced with new one so remove it from pmap. */
  vaddr_t fault_page = off * PAGESIZE + ent->start;
  pmap_remove(map->pmap, fault_page, fault_page + PAGESIZE);

  /* Replace anon */
  vm_anon_t *new = vm_anon_copy(*anonp);
  vm_anon_t *old = vm_amap_insert_anon(ent->aref, new, off);

  /* Check if replaced anon is the one we expect to be replaced. */
  assert(*anonp == old);
  vm_anon_drop(old);

  *ins_protp = ent->prot;
  *anonp = new;
  return 0;
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

  if (!(ent->prot & VM_PROT_WRITE) && (fault_type & VM_PROT_WRITE)) {
    klog("Cannot write to address: 0x%08lx", fault_addr);
    return EACCES;
  }

  if (!(ent->prot & VM_PROT_READ) && (fault_type & VM_PROT_READ)) {
    klog("Cannot read from address: 0x%08lx", fault_addr);
    return EACCES;
  }

  if (!(ent->prot & VM_PROT_EXEC) && (fault_type & VM_PROT_EXEC)) {
    klog("Cannot exec at address: 0x%08lx", fault_addr);
    return EACCES;
  }

  assert(ent->start <= fault_addr && fault_addr < ent->end);

  vm_anon_t *anon = NULL;
  vaddr_t fault_page = fault_addr & -PAGESIZE;
  size_t offset = vaddr_to_slot(fault_page - ent->start);

  if (ent->aref.amap) {
    /* Look for anon in existing amap. */
    anon = vm_amap_find_anon(ent->aref, offset);
  } else {
    /* Create a new amap. */
    anon = NULL;
    size_t amap_slots = vaddr_to_slot(ent->end - ent->start);
    ent->aref.amap = vm_amap_alloc(amap_slots);
    ent->aref.offset = 0;
  }

  int err;
  vm_prot_t insert_prot;

  if (ent->flags & VM_ENT_COW)
    err = cow_page_fault(map, ent, offset, &anon, &insert_prot, fault_type);
  else
    err = insert_anon(ent, offset, &anon, &insert_prot);

  if (err)
    return err;

  pmap_enter(map->pmap, fault_page, anon->page, insert_prot, 0);
  return 0;
}
