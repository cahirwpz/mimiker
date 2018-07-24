#include <rman.h>
#include <pool.h>

static POOL_DEFINE(P_RES, "resources", sizeof(resource_t));

static rman_addr_t rman_find_gap(rman_t *rm, rman_addr_t start, rman_addr_t end,
                                 size_t count, size_t bound,
                                 resource_t **res_p) {
  assert(mtx_owned(&rm->rm_lock));

  if (end < rm->rm_start || start >= rm->rm_end)
    return 0;

  /* Adjust search boundaries if needed. */
  start = max(start, rm->rm_start);
  end = min(end, rm->rm_end);

  /* Fits before the first resource on the list ? */
  resource_t *first_r = TAILQ_FIRST(&rm->rm_resources);
  rman_addr_t first_start = first_r ? first_r->r_start : rm->rm_end + 1;
  if (align(start, bound) + count <= first_start)
    return align(start, bound);

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
      *res_p = curr_r;
      return align(first, bound);
    }
  }

  return 0;
}

resource_t *rman_alloc_resource(rman_t *rm, rman_addr_t first, rman_addr_t last,
                                size_t count, size_t bound, res_flags_t flags,
                                device_t *dev) {
  assert(first <= last);
  assert(powerof2(bound) && (bound > 0));

  resource_t *r = pool_alloc(P_RES, PF_ZERO);
  r->r_type = rm->rm_type;
  r->r_flags = flags;
  r->r_owner = dev;
  /* TODO insert onto list of resources that are owned by the device */

  WITH_MTX_LOCK (&rm->rm_lock) {
    resource_t *after = NULL;
    rman_addr_t start;
    if ((start = rman_find_gap(rm, first, last, count, bound, &after))) {
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
      pool_free(P_RES, r);
      r = NULL;
    }
  }

  return r;
}

void rman_create(rman_t *rm, rman_addr_t start, rman_addr_t end,
                 res_type_t type) {
  rm->rm_start = start;
  rm->rm_end = end;
  rm->rm_type = type;

  mtx_init(&rm->rm_lock, MTX_DEF);
  TAILQ_INIT(&rm->rm_resources);
}

void rman_create_from_resource(rman_t *rm, resource_t *res) {
  rman_create(rm, res->r_start, res->r_end, res->r_type);
}
