#include <sys/mimiker.h>
#include <sys/rman.h>
#include <sys/malloc.h>

typedef TAILQ_HEAD(, resource) res_list_t;

struct rman_region {
  rman_addr_t start;             /* first physical address */
  rman_addr_t end;               /* last physical address */
  res_list_t resources;          /* all managed resources */
  TAILQ_ENTRY(rman_region) link; /* link on resource manager list */
};

static KMALLOC_DEFINE(M_RES, "resources & regions");

static bool rman_region_find_gap(rman_region_t *reg, rman_addr_t *start_p,
                                 rman_addr_t end, size_t count, size_t bound,
                                 resource_t **res_p) {
  rman_addr_t start = *start_p;

  if (end < reg->start || start >= reg->end)
    return false;

  /* Adjust search boundaries if needed. */
  start = max(start, reg->start);
  end = min(end, reg->end);

  /* Fits before the first resource on the list ? */
  resource_t *first_r = TAILQ_FIRST(&reg->resources);
  rman_addr_t first_start = first_r ? first_r->r_start : reg->end + 1;
  if (align(start, bound) + count <= first_start) {
    *start_p = align(start, bound);
    return true;
  }

  /* Look up first element after which we can place our new resource. */
  resource_t *curr_r, *next_r;
  TAILQ_FOREACH_SAFE (curr_r, &reg->resources, r_link, next_r) {
    rman_addr_t curr_end = curr_r->r_end + 1;
    rman_addr_t next_start = next_r ? next_r->r_start : reg->end + 1;

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

/* Place a resource within a resource manager region. */
static bool rman_region_place_resource(rman_region_t *reg, rman_addr_t first,
                                       rman_addr_t last, size_t count,
                                       size_t bound, resource_t *r) {
  resource_t *after = NULL;
  rman_addr_t start = first;

  if (rman_region_find_gap(reg, &start, last, count, bound, &after)) {
    assert(start >= reg->start && start >= first && start < last);
    assert(start + count - 1 <= last);
    assert(start + count - 1 <= reg->end);
    assert(is_aligned(start, bound));
    r->r_start = start;
    r->r_end = start + count - 1;
    if (after)
      TAILQ_INSERT_AFTER(&reg->resources, after, r, r_link);
    else
      TAILQ_INSERT_HEAD(&reg->resources, r, r_link);
    return true;
  }

  return false;
}

resource_t *rman_alloc_resource(rman_t *rm, rman_addr_t first, rman_addr_t last,
                                size_t count, size_t bound, res_flags_t flags,
                                device_t *dev) {
  assert(first <= last);
  assert(powerof2(bound) && (bound > 0));

  if (align(first, bound) + count > last + 1)
    return NULL;

  resource_t *r = kmalloc(M_RES, sizeof(resource_t), M_ZERO);
  r->r_rman = rm;
  r->r_type = rm->rm_type;
  r->r_flags = flags;

  bool placed = false;

  WITH_MTX_LOCK (&rm->rm_lock) {
    rman_region_t *reg;
    /* Find first suitable region. */
    TAILQ_FOREACH (reg, &rm->rm_regions, link) {
      if (last + 1 <= reg->start)
        break;
      if (reg->end - reg->start + 1 < count)
        continue;
      if ((placed =
             rman_region_place_resource(reg, first, last, count, bound, r)))
        break;
    }
  }

  if (!placed) {
    kfree(M_RES, r);
    r = NULL;
  }

  return r;
}

void rman_manage_region(rman_t *rm, rman_addr_t start, rman_addr_t end) {
  rman_region_t *reg;

  reg = kmalloc(M_RES, sizeof(rman_region_t), M_ZERO);
  TAILQ_INIT(&reg->resources);
  reg->start = start;
  reg->end = end;

  WITH_MTX_LOCK (&rm->rm_lock) {
    rman_addr_t max_addr = ~0;
    rman_region_t *p, *s;
    /* Skip entries before us. */
    TAILQ_FOREACH (p, &rm->rm_regions, link) {
      if (p->end + 1 == max_addr)
        break;
      if (p->end + 1 >= reg->start)
        break;
    }
    /* If we ran off the end of the list, insert at the tail. */
    if (!p) {
      TAILQ_INSERT_TAIL(&rm->rm_regions, reg, link);
    } else {
      /* Check for any overlap with the current region. */
      assert(!(reg->start <= p->end && reg->end >= p->start));

      /* Check for any overlap with the next region. */
      s = TAILQ_NEXT(p, link);
      if (s)
        assert(!(reg->start <= s->end && reg->end >= s->start));

      /* Insert the new region. */
      if (p->end < reg->start)
        TAILQ_INSERT_AFTER(&rm->rm_regions, p, reg, link);
      else
        TAILQ_INSERT_BEFORE(p, reg, link);
    }
  }
}

void rman_release_resource(resource_t *r) {
  rman_t *rm = r->r_rman;

  WITH_MTX_LOCK (&rm->rm_lock) {
    rman_region_t *reg;
    TAILQ_FOREACH (reg, &rm->rm_regions, link) {
      if (r->r_start >= reg->start && r->r_end <= reg->end)
        break;
    }
    assert(reg);
    TAILQ_REMOVE(&reg->resources, r, r_link);
  }

  kfree(M_RES, r);
}

void rman_activate_resource(resource_t *r) {
  rman_t *rm = r->r_rman;

  WITH_MTX_LOCK (&rm->rm_lock)
    r->r_flags |= RF_ACTIVE;
}

void rman_init(rman_t *rm, const char *name, res_type_t type) {
  rm->rm_name = name;
  rm->rm_type = type;

  mtx_init(&rm->rm_lock, 0);
  TAILQ_INIT(&rm->rm_regions);
}

void rman_destroy(rman_t *rm) {
  rman_region_t *reg;
  resource_t *r;

  WITH_MTX_LOCK (&rm->rm_lock) {
    while (!TAILQ_EMPTY(&rm->rm_regions)) {
      reg = TAILQ_FIRST(&rm->rm_regions);

      while (!TAILQ_EMPTY(&reg->resources)) {
        r = TAILQ_FIRST(&reg->resources);
        TAILQ_REMOVE(&reg->resources, r, r_link);
        kfree(M_RES, r);
      }

      TAILQ_REMOVE(&rm->rm_regions, reg, link);
      kfree(M_RES, reg);
    }
  }
  /* TODO: destroy the `rm_lock` after implementing `mtx_desotry`. */
}
