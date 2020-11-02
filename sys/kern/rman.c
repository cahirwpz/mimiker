#include <sys/mimiker.h>
#include <sys/rman.h>
#include <sys/malloc.h>

static KMALLOC_DEFINE(M_RES, "resources");

static bool rman_find_gap(rman_t *rm, rman_addr_t *start_p, rman_addr_t end,
                          size_t count, size_t bound, resource_t **res_p) {
  assert(mtx_owned(&rm->rm_lock));

  rman_addr_t start = *start_p;

  if (end < rm->rm_start || start >= rm->rm_end)
    return false;

  /* Adjust search boundaries if needed. */
  start = max(start, rm->rm_start);
  end = min(end, rm->rm_end);

  /* Fits before the first resource on the list ? */
  resource_t *first_r = TAILQ_FIRST(&rm->rm_resources);
  rman_addr_t first_start = first_r ? first_r->r_start : rm->rm_end + 1;
  if (align(start, bound) + count <= first_start) {
    *start_p = align(start, bound);
    return true;
  }

  /* Look up first element after which we can place our new resource. */
  resource_t *curr_r = NULL;
  TAILQ_FOREACH (curr_r, &rm->rm_resources, r_link) {
    resource_t *next_r = TAILQ_NEXT(curr_r, r_link);
    rman_addr_t curr_end = curr_r->r_end + 1;
    rman_addr_t next_start = next_r ? next_r->r_start : rm->rm_end + 1;

    /* Skip elements before `start` address. */
    if (start >= next_start)
      continue;

    /* Do not consider elements after `end` address. */
    if (end <= curr_end)
      break;

    /* Calculate first & last address of eligible gap. */
    rman_addr_t first = align(max(curr_end, start), bound);
    rman_addr_t last = min(next_start - 1, end);

    /* Does region of `count` size fit in the gap at `bound` aligned address? */
    if (first + count - 1 <= last) {
      *start_p = align(first, bound);
      *res_p = curr_r;
      return true;
    }
  }

  return false;
}

resource_t *rman_alloc_resource(rman_t *rm, rman_addr_t first, rman_addr_t last,
                                size_t count, size_t bound, res_flags_t flags,
                                device_t *dev) {
  assert(first <= last);
  assert(powerof2(bound) && (bound > 0));

  resource_t *r = kmalloc(M_RES, sizeof(resource_t), M_ZERO);
  r->r_rman = rm;
  r->r_type = rm->rm_type;
  r->r_flags = flags;

  WITH_MTX_LOCK (&rm->rm_lock) {
    resource_t *after = NULL;
    rman_addr_t start = first;
    if (rman_find_gap(rm, &start, last, count, bound, &after)) {
      assert(start >= first && start < last);
      assert(start + count - 1 <= last);
      assert(is_aligned(start, bound));
      r->r_start = start;
      r->r_end = start + count - 1;
      if (after)
        TAILQ_INSERT_AFTER(&rm->rm_resources, after, r, r_link);
      else
        TAILQ_INSERT_HEAD(&rm->rm_resources, r, r_link);
    } else {
      kfree(M_RES, r);
      r = NULL;
    }
  }

  return r;
}

void rman_release_resource(resource_t *r) {
  rman_t *rm = r->r_rman;

  WITH_MTX_LOCK (&rm->rm_lock)
    TAILQ_REMOVE(&rm->rm_resources, r, r_link);

  kfree(M_RES, r);
}

void rman_activate_resource(resource_t *r) {
  rman_t *rm = r->r_rman;

  WITH_MTX_LOCK (&rm->rm_lock)
    r->r_flags |= RF_ACTIVE;
}

void rman_init(rman_t *rm, const char *name, rman_addr_t start, rman_addr_t end,
               res_type_t type) {
  rm->rm_name = name;
  rm->rm_start = start;
  rm->rm_end = end;
  rm->rm_type = type;

  mtx_init(&rm->rm_lock, 0);
  TAILQ_INIT(&rm->rm_resources);
}
