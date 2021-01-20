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
#include <sys/refcnt.h>
#include <sys/rman.h>
#include <sys/malloc.h>

struct range {
  rman_t *rman;            /* manager of this range */
  rman_addr_t start;       /* first physical address of the range */
  rman_addr_t end;         /* last (inclusive) physical address */
  rman_flags_t flags;      /* or'ed RF_* values */
  int refcnt;              /* number of sharers */
  refcnt_t active_cnt;     /* number of activations performed on the range */
  TAILQ_ENTRY(range) link; /* link on range manager list */
};

static KMALLOC_DEFINE(M_RMAN, "rman ranges");

static int rman_release_range(range_t *r);

void rman_init(rman_t *rm, const char *name) {
  rm->rm_name = name;
  mtx_init(&rm->rm_lock, 0);
  TAILQ_INIT(&rm->rm_ranges);
}

static range_t *range_alloc(rman_t *rm, rman_addr_t start, rman_addr_t end,
                            rman_flags_t flags) {
  range_t *r = kmalloc(M_RMAN, sizeof(range_t), M_ZERO);
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

static int r_shareable(range_t *r) {
  return r->flags & RF_SHAREABLE;
}

static size_t r_size(range_t *r) {
  return r->end - r->start + 1;
}

static int r_canmerge(range_t *r, range_t *next) {
  return (r->end + 1 == next->start) && !r_reserved(next);
}

static int r_canshare(range_t *r, rman_addr_t start, size_t count,
                      rman_flags_t flags) {
  return r_shareable(r) && (flags & RF_SHAREABLE) && r_size(r) == count &&
         r->start >= start;
}

void rman_manage_region(rman_t *rm, rman_addr_t start, size_t size) {
  rman_addr_t end = start + size - 1;
  range_t *reg = range_alloc(rm, start, end, RF_RESERVED);

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
    assert(!r_reserved(r)); /* can't free range that's in use */
    TAILQ_REMOVE(&rm->rm_ranges, r, link);
    kfree(M_RMAN, r);
  }

  /* TODO: destroy the `rm_lock` after implementing `mtx_destroy`. */
}

static range_t *rman_reserve_range(rman_t *rm, rman_addr_t start,
                                   rman_addr_t end, size_t count,
                                   size_t alignment, rman_flags_t flags) {
  assert(start + count - 1 <= end);
  assert(count > 0);

  alignment = max(alignment, 1UL);
  assert(powerof2(alignment));

  SCOPED_MTX_LOCK(&rm->rm_lock);

  range_t *r;
  TAILQ_FOREACH (r, &rm->rm_ranges, link) {
    if (r_reserved(r)) {
      if (r_canshare(r, start, count, flags)) {
        r->refcnt++;
        return r;
      }
      continue;
    }

    rman_addr_t new_start = roundup(max(r->start, start), alignment);
    rman_addr_t new_end = new_start + count - 1;

    /* See if it fits. */
    if (new_end > r->end)
      continue;

    /* Isn't it too far? */
    if (new_end > end)
      break;

    /* Truncate the range from both sides. This may create up to two new range
     * before and after the chosen one. */
    if (r->start < new_start) {
      range_t *prev = range_alloc(rm, r->start, new_start - 1, r->flags);
      TAILQ_INSERT_BEFORE(r, prev, link);
      r->start = new_start;
    }

    if (r->end > new_end) {
      range_t *next = range_alloc(rm, new_end + 1, r->end, r->flags);
      TAILQ_INSERT_AFTER(&rm->rm_ranges, r, next, link);
      r->end = new_end;
    }

    /* Finally... we're left with range we were exactly looking for.
     * Just mark is as reserved and add extra flags like RF_PREFETCHABLE. */
    r->flags |= RF_RESERVED;
    r->flags |= flags & ~RF_ACTIVE;
    return r;
  }

  return NULL;
}

/*! \brief Removes a range from its range manager and releases memory.
 *
 * \returns 0 if the range structure must be released. */
static int rman_release_range(range_t *r) {
  rman_t *rm = r->rman;

  SCOPED_MTX_LOCK(&rm->rm_lock);

  assert(r_reserved(r));

  if (r_shareable(r) && --r->refcnt)
    return 1;

  assert(!r_active(r));

  /*
   * Look at the adjacent range in the list and see if our range can be merged
   * with any of them. If either of the ranges is reserved or is not adjacent
   * then they cannot be merged with our range.
   */
  range_t *prev = TAILQ_PREV(r, range_list, link);
  if (prev && r_canmerge(prev, r)) {
    /* Merge previous region with ours. */
    prev->end = r->end;
    TAILQ_REMOVE(&rm->rm_ranges, r, link);
    kfree(M_RMAN, r);
    r = prev;
  }

  range_t *next = TAILQ_NEXT(r, link);
  if (next && r_canmerge(r, next)) {
    /* Merge next region with ours. */
    r->end = next->end;
    TAILQ_REMOVE(&rm->rm_ranges, next, link);
    kfree(M_RMAN, next);
  }

  /* Merging is done... we simply mark the region as free. */
  r->flags = 0;
  return 0;
}

resource_t *rman_reserve_resource(rman_t *rm, res_type_t type, int rid,
                                  rman_addr_t start, rman_addr_t end,
                                  size_t size, size_t alignment,
                                  rman_flags_t flags) {
  range_t *range = rman_reserve_range(rm, start, end, size, alignment, flags);
  if (!range)
    return NULL;

  resource_t *r = kmalloc(M_RMAN, sizeof(resource_t), M_WAITOK);
  r->r_range = range;
  r->r_type = type;
  r->r_rid = rid;
  return r;
}

bus_size_t resource_size(resource_t *r) {
  return r_size(r->r_range);
}

rman_addr_t resource_start(resource_t *r) {
  return r->r_range->start;
}

bool resource_active(resource_t *r) {
  return r->r_range->flags & RF_ACTIVE;
}

void resource_activate(resource_t *res) {
  range_t *r = res->r_range;
  WITH_MTX_LOCK (&r->rman->rm_lock) {
    r->flags |= RF_ACTIVE;
    refcnt_acquire(&r->active_cnt);
  }
}

void resource_deactivate(resource_t *res) {
  range_t *r = res->r_range;
  if (refcnt_release(&r->active_cnt))
    WITH_MTX_LOCK (&r->rman->rm_lock) { r->flags &= ~RF_ACTIVE; }
}

/*! \brief Removes a range from its range manager and releases memory. */
void resource_release(resource_t *r) {
  if (!rman_release_range(r->r_range))
    kfree(M_RMAN, r);
}
