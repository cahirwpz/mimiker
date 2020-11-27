/*
 * The kernel resource manager.
 *
 * Heavily inspired by FreeBSD's resource manager.
 *
 * This code is responsible for keeping track of hardware resources which
 * are apportioned out to various drivers. It does not actually assign those
 * resources, and it is not expected that end-device drivers will call into
 * this code directly.  Rather, the code which implements the buses that those
 * devices are attached to, and the code which manages CPU resources, will call
 * this code, and the end-device drivers will make upcalls to that code to
 * actually perform the allocation.
 */

#include <sys/mimiker.h>
#include <sys/rman.h>
#include <sys/malloc.h>

static KMALLOC_DEFINE(M_RES, "resources & regions");

void rman_init(rman_t *rm, const char *name) {
  rm->rm_name = name;
  mtx_init(&rm->rm_lock, 0);
  TAILQ_INIT(&rm->rm_resources);
}

static resource_t *resource_alloc(rman_t *rm, rman_addr_t start,
                                  rman_addr_t end, res_flags_t flags) {
  resource_t *r = kmalloc(M_RES, sizeof(resource_t), M_ZERO);
  r->r_rman = rm;
  r->r_start = start;
  r->r_end = end;
  r->r_flags = flags;
  return r;
}

static int r_active(resource_t *r) {
  return r->r_flags & RF_ACTIVE;
}

static int r_reserved(resource_t *r) {
  return r->r_flags & RF_RESERVED;
}

static int r_overlap(resource_t *curr, resource_t *with) {
  return curr->r_start <= with->r_end && curr->r_end >= with->r_start;
}

static int r_canmerge(resource_t *curr, resource_t *next) {
  return (curr->r_end + 1 == next->r_start) && !r_reserved(next);
}

void rman_manage_region(rman_t *rm, rman_addr_t start, size_t size) {
  assert((start + size - 1) >= start); /* check for overflow */

  SCOPED_MTX_LOCK(&rm->rm_lock);

  resource_t *r = resource_alloc(rm, start, start + size - 1, RF_NONE);
  resource_t *cur;

  /* Skip entries before us. */
  TAILQ_FOREACH (cur, &rm->rm_resources, r_link) {
    /* Note that we need this aditional check
     * due to possible overflow in the following case. */
    if (cur->r_end == RMAN_ADDR_MAX)
      break;
    if (cur->r_end + 1 >= r->r_start)
      break;
  }

  /* If we ran off the end of the list, insert at the tail. */
  if (!cur) {
    TAILQ_INSERT_TAIL(&rm->rm_resources, r, r_link);
    return;
  }

  assert(!r_overlap(r, cur));

  resource_t *next = TAILQ_NEXT(cur, r_link);
  if (next) {
    assert(!r_overlap(r, next));

    /* See if the new region can be merged with the next region.
     * If not, clear the pointer. */
    if (!r_canmerge(r, next))
      next = NULL;
  }

  /* See if we can merge with the current region. */
  if (cur->r_end != RMAN_ADDR_MAX && /* watch out for overflow */
      r_canmerge(cur, r)) {
    /* Can we merge all 3 regions? */
    if (next) {
      cur->r_end = next->r_end;
      TAILQ_REMOVE(&rm->rm_resources, next, r_link);
      kfree(M_RES, r);
      kfree(M_RES, next);
    } else {
      cur->r_end = r->r_end;
      kfree(M_RES, r);
    }
  } else if (next) {
    /* We can merge with just the next region. */
    next->r_start = r->r_start;
    kfree(M_RES, r);
  } else if (cur->r_end < r->r_start) {
    TAILQ_INSERT_AFTER(&rm->rm_resources, cur, r, r_link);
  } else {
    TAILQ_INSERT_BEFORE(cur, r, r_link);
  }
}

void rman_init_from_resource(rman_t *rm, const char *name, resource_t *r) {
  rman_init(rm, name);
  rman_manage_region(rm, r->r_start, resource_size(r));
}

void rman_fini(rman_t *rm) {
  SCOPED_MTX_LOCK(&rm->rm_lock);

  while (!TAILQ_EMPTY(&rm->rm_resources)) {
    resource_t *r = TAILQ_FIRST(&rm->rm_resources);
    assert(!r_reserved(r)); /* can't free resource that's in use */
    TAILQ_REMOVE(&rm->rm_resources, r, r_link);
    kfree(M_RES, r);
  }

  /* TODO: destroy the `rm_lock` after implementing `mtx_destroy`. */
}

resource_t *rman_reserve_resource(rman_t *rm, rman_addr_t start,
                                  rman_addr_t end, size_t count,
                                  size_t alignment, res_flags_t flags) {
  /* Watch out for overflow. */
  assert((start + count - 1) >= start);
  assert((start + count - 1) <= end);
  assert(count > 0);

  alignment = max(alignment, 1UL);
  assert(powerof2(alignment));

  SCOPED_MTX_LOCK(&rm->rm_lock);

  resource_t *r;
  TAILQ_FOREACH (r, &rm->rm_resources, r_link) {
    /* Skip lower regions. */
    if (r->r_end < start + count - 1)
      continue;

    /* Skip reserved regions. */
    if (r_reserved(r))
      continue;

    /* Stop if we've gone too far. */
    if (r->r_start > end - count + 1)
      break;

    /* Stop if roundup causes overflow. */
    if (r->r_start > RMAN_ADDR_MAX - alignment + 1)
      break;

    rman_addr_t new_start = roundup(max(r->r_start, start), alignment);
    rman_addr_t new_end = new_start + count - 1;

    /* Check for overflow. */
    if ((new_start + count - 1) < new_start)
      break;

    /* See if it fits. */
    if (new_end > r->r_end)
      continue;

    /* Isn't it too far? */
    if (new_end > end)
      break;

    /* Truncate the resource from both sides. This may create up to two new
     * resource before and after the chosen one. */
    if (r->r_start < new_start) {
      resource_t *prev =
        resource_alloc(rm, r->r_start, new_start - 1, r->r_flags);
      TAILQ_INSERT_BEFORE(r, prev, r_link);
      r->r_start = new_start;
    }

    if (r->r_end > new_end) {
      resource_t *next = resource_alloc(rm, new_end + 1, r->r_end, r->r_flags);
      TAILQ_INSERT_AFTER(&rm->rm_resources, r, next, r_link);
      r->r_end = new_end;
    }

    /* Finally... we're left with resource we were exactly looking for.
     * Just mark is as reserved and add extra flags like RF_PREFETCHABLE. */
    r->r_flags |= RF_RESERVED;
    r->r_flags |= flags & ~RF_ACTIVE;
    return r;
  }

  return NULL;
}

void rman_activate_resource(resource_t *r) {
  WITH_MTX_LOCK (&r->r_rman->rm_lock) { r->r_flags |= RF_ACTIVE; }
}

void rman_deactivate_resource(resource_t *r) {
  WITH_MTX_LOCK (&r->r_rman->rm_lock) { r->r_flags &= ~RF_ACTIVE; }
}

void rman_release_resource(resource_t *r) {
  rman_t *rm = r->r_rman;

  SCOPED_MTX_LOCK(&rm->rm_lock);

  assert(!r_active(r));

  /*
   * Look at the adjacent resources in the list and see if our resource
   * can be merged with any of them. If either of the resources is reserved
   * or is not adjacent then they cannot be merged with our resource.
   */
  resource_t *prev = TAILQ_PREV(r, res_list, r_link);
  if (prev && r_canmerge(prev, r)) {
    /* Merge previous region with ours. */
    prev->r_end = r->r_end;
    TAILQ_REMOVE(&rm->rm_resources, r, r_link);
    kfree(M_RES, r);
    r = prev;
  }

  resource_t *next = TAILQ_NEXT(r, r_link);
  if (next && r_canmerge(r, next)) {
    /* Merge next region with ours. */
    r->r_end = next->r_end;
    TAILQ_REMOVE(&rm->rm_resources, next, r_link);
    kfree(M_RES, next);
  }

  /* Merging is done... we simply mark the region as free. */
  r->r_flags &= ~RF_RESERVED;
}
