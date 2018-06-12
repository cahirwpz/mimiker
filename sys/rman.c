#include <rman.h>
#include <stdc.h>
#include <pool.h>

static pool_t P_RMAN;

static resource_t *find_resource(rman_t *rm, rman_addr_t start, rman_addr_t end,
                                 size_t count, size_t align, unsigned flags) {
  resource_t *resource;
  LIST_FOREACH(resource, &rm->rm_resources, r_resources) {
    if (resource->r_flags & RF_ALLOCATED &&
        !((resource->r_flags & RF_SHARED) && (flags & RF_SHARED)))
      continue;

    if (start > resource->r_end || end < resource->r_start) {
      continue;
    }

    /* calculate common part and check if is big enough */
    rman_addr_t s = align(max(start, resource->r_start), align);
    rman_addr_t e = min(end, resource->r_end);

    rman_addr_t len = e - s + 1;

    /* when trying to use existing resource, size should be the same
     */
    if (flags & RF_SHARED)
      if ((rman_addr_t)count != resource->r_end - resource->r_start + 1)
        continue;

    if (len >= (rman_addr_t)count) {
      return resource;
    }
  }

  return NULL;
}

/* divide resource into two
 `where` means start of right resource
 function returns pointer to new (right) resource */
static resource_t *cut_resource(resource_t *resource, rman_addr_t where) {
  assert(where > resource->r_start);
  assert(where < resource->r_end);

  resource_t *left_resource = resource;

  resource_t *right_resource = pool_alloc(P_RMAN, 0);
  memset(right_resource, 0, sizeof(resource_t));

  left_resource->r_resources.le_next = right_resource;
  right_resource->r_resources.le_prev = &left_resource;

  right_resource->r_end = left_resource->r_end;
  right_resource->r_start = where;
  left_resource->r_end = where - 1;

  return right_resource;
}

/* maybe split resource into two or three in order to recover space before and
 after allocation */
static resource_t *split_resource(resource_t *resource, rman_addr_t start,
                                  rman_addr_t end, size_t count) {
  if (resource->r_start < start) {
    resource = cut_resource(resource, start);
  }

  if (resource->r_end > resource->r_start + (rman_addr_t)count - 1) {
    cut_resource(resource, resource->r_start + count);
  }

  return resource;
}

resource_t *rman_allocate_resource(rman_t *rm, rman_addr_t start,
                                   rman_addr_t end, size_t count, size_t align,
                                   unsigned flags) {
  SCOPED_MTX_LOCK(&rm->rm_mtx);

  align = min(align, sizeof(void *));

  resource_t *resource = find_resource(rm, start, end, count, align, flags);
  if (resource == NULL) {
    return NULL;
  }

  resource = split_resource(resource, align(start, align), end, count);
  resource->r_align = align;
  resource->r_flags = flags;
  resource->r_flags |= RF_ALLOCATED;

  return resource;
}

void rman_create(rman_t *rm, rman_addr_t start, rman_addr_t end) {
  rm->rm_start = start;
  rm->rm_end = end;

  mtx_init(&rm->rm_mtx, MTX_DEF);
  LIST_INIT(&rm->rm_resources);

  resource_t *whole_space = pool_alloc(P_RMAN, 0);
  memset(whole_space, 0, sizeof(resource_t));

  whole_space->r_start = rm->rm_start;
  whole_space->r_end = rm->rm_end;

  LIST_INSERT_HEAD(&rm->rm_resources, whole_space, r_resources);
}

void rman_init(void) {
  P_RMAN = pool_create("rman", sizeof(resource_t), NULL, NULL);
}
