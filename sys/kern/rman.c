/*
 * The kernel range manager for resources.
 *
 * Heavily inspired by FreeBSD's resource manager.
 *
 * This code is responsible for keeping track of hardware address ranges which
 * are apportioned out to various drivers. It does not actually assign those
 * ranges, and it is not expected that end-device drivers will call into this
 * code directly.  Rather, the code which implements the buses that those
 * devices are attached to, and the code which manages CPU resources, will call
 * this code, and the end-device drivers will make upcalls to that code to
 * actually perform the allocation.
 */

#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/rman.h>
#include <sys/malloc.h>

static KMALLOC_DEFINE(M_RES, "rman ranges");

void rman_init(rman_t *rm, const char *name) {
  rm->rm_name = name;
  mtx_init(&rm->rm_lock, 0);
  TAILQ_INIT(&rm->rm_ranges);
}

static range_t *resource_alloc(rman_t *rm, rman_addr_t start, rman_addr_t end,
                               rman_flags_t flags) {
  range_t *r = kmalloc(M_RES, sizeof(range_t), M_ZERO);
  r->rman = rm;
  r->start = start;
  r->end = end;
  r->flags = flags;
  return r;
}

static int r_active(range_t *r) {
  return r->flags & RF_ACTIVE;
}

static int r_reserved(range_t *r) {
  return r->flags & RF_RESERVED;
}

static int r_canmerge(range_t *r, range_t *next) {
  return (r->end + 1 == next->start) && !r_reserved(next);
}

void rman_manage_region(rman_t *rm, rman_addr_t start, size_t size) {
  rman_addr_t end = start + size - 1;
  range_t *reg = resource_alloc(rm, start, end, RF_RESERVED);

  WITH_MTX_LOCK (&rm->rm_lock) {
    range_t *first = TAILQ_FIRST(&rm->rm_ranges);
    if (first == NULL || end < first->start) {
      TAILQ_INSERT_HEAD(&rm->rm_ranges, reg, link);
      goto done;
    }

    range_t *last = TAILQ_LAST(&rm->rm_ranges, range_list);
    if (start > last->end) {
      TAILQ_INSERT_TAIL(&rm->rm_ranges, reg, link);
      goto done;
    }

    range_t *next;
    for (range_t *r = first; r != last; r = next) {
      next = TAILQ_NEXT(r, link);
      if (r->end < start && end < next->start) {
        TAILQ_INSERT_AFTER(&rm->rm_ranges, r, reg, link);
        goto done;
      }
    }

    panic("overlapping regions");
  }

done:
  rman_release_range(reg);
}

void rman_init_from_resource(rman_t *rm, const char *name, resource_t *r) {
  rman_init(rm, name);
  rman_manage_region(rm, resource_start(r), resource_size(r));
}

void rman_fini(rman_t *rm) {
  SCOPED_MTX_LOCK(&rm->rm_lock);

  while (!TAILQ_EMPTY(&rm->rm_ranges)) {
    range_t *r = TAILQ_FIRST(&rm->rm_ranges);
    assert(!r_reserved(r)); /* can't free resource that's in use */
    TAILQ_REMOVE(&rm->rm_ranges, r, link);
    kfree(M_RES, r);
  }

  /* TODO: destroy the `rm_lock` after implementing `mtx_destroy`. */
}

range_t *rman_reserve_range(rman_t *rm, rman_addr_t start, rman_addr_t end,
                            size_t count, size_t alignment,
                            rman_flags_t flags) {
  assert(start + count - 1 <= end);
  assert(count > 0);

  alignment = max(alignment, 1UL);
  assert(powerof2(alignment));

  SCOPED_MTX_LOCK(&rm->rm_lock);

  range_t *r;
  TAILQ_FOREACH (r, &rm->rm_ranges, link) {
    /* Skip reserved regions. */
    if (r_reserved(r))
      continue;

    rman_addr_t new_start = roundup(max(r->start, start), alignment);
    rman_addr_t new_end = new_start + count - 1;

    /* See if it fits. */
    if (new_end > r->end)
      continue;

    /* Isn't it too far? */
    if (new_end > end)
      break;

    /* Truncate the resource from both sides. This may create up to two new
     * resource before and after the chosen one. */
    if (r->start < new_start) {
      range_t *prev = resource_alloc(rm, r->start, new_start - 1, r->flags);
      TAILQ_INSERT_BEFORE(r, prev, link);
      r->start = new_start;
    }

    if (r->end > new_end) {
      range_t *next = resource_alloc(rm, new_end + 1, r->end, r->flags);
      TAILQ_INSERT_AFTER(&rm->rm_ranges, r, next, link);
      r->end = new_end;
    }

    /* Finally... we're left with resource we were exactly looking for.
     * Just mark is as reserved and add extra flags like RF_PREFETCHABLE. */
    r->flags |= RF_RESERVED;
    r->flags |= flags & ~RF_ACTIVE;
    return r;
  }

  return NULL;
}

void rman_activate_range(range_t *r) {
  WITH_MTX_LOCK (&r->rman->rm_lock) { r->flags |= RF_ACTIVE; }
}

void rman_deactivate_range(range_t *r) {
  WITH_MTX_LOCK (&r->rman->rm_lock) { r->flags &= ~RF_ACTIVE; }
}

void rman_release_range(range_t *r) {
  rman_t *rm = r->rman;

  SCOPED_MTX_LOCK(&rm->rm_lock);

  assert(!r_active(r));
  assert(r_reserved(r));

  /*
   * Look at the adjacent resources in the list and see if our resource
   * can be merged with any of them. If either of the resources is reserved
   * or is not adjacent then they cannot be merged with our resource.
   */
  range_t *prev = TAILQ_PREV(r, range_list, link);
  if (prev && r_canmerge(prev, r)) {
    /* Merge previous region with ours. */
    prev->end = r->end;
    TAILQ_REMOVE(&rm->rm_ranges, r, link);
    kfree(M_RES, r);
    r = prev;
  }

  range_t *next = TAILQ_NEXT(r, link);
  if (next && r_canmerge(r, next)) {
    /* Merge next region with ours. */
    r->end = next->end;
    TAILQ_REMOVE(&rm->rm_ranges, next, link);
    kfree(M_RES, next);
  }

  /* Merging is done... we simply mark the region as free. */
  r->flags = 0;
}
