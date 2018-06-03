#include <rman.h>

// TODO all this logic associated with resources is not really tested

static resource_t *find_resource(rman_t *rm, rm_res_t start, rm_res_t end,
                                 rm_res_t count) {
  for (resource_t *resource = rm->resources.lh_first; resource != NULL;
       resource = resource->resources.le_next) {
    if (resource->r_flags & RF_ALLOCATED)
      continue;

    // calculate common part and check if is big enough
    rm_res_t s = max(start, resource->r_start);
    rm_res_t e = min(end, resource->r_end);

    rm_res_t len = s - e + 1;

    if (len >= count) {
      return resource;
    }
  }

  return NULL;
}

// divide resource into two
// `where` means start of right resource
// function returns pointer to new (right) resource
static resource_t *cut_resource(resource_t *resource, rm_res_t where) {
  assert(where > resource->r_start);
  assert(where < resource->r_end);

  resource_t *left_resource = resource;
  resource_t *right_resource = kmalloc(M_DEV, sizeof(resource_t), M_ZERO);

  left_resource->resources.le_next = right_resource;
  right_resource->resources.le_prev = &left_resource;

  right_resource->r_end = left_resource->r_end;
  right_resource->r_start = where;
  left_resource->r_end = where - 1;

  return right_resource;
}

// maybe split resource into two or three in order to recover space before and
// after allocation
static resource_t *split_resource(resource_t *resource, rm_res_t start,
                                  rm_res_t end, rm_res_t count) {
  if (resource->r_start < start) {
    resource = cut_resource(resource, start);
  }

  if (resource->r_end > resource->r_start + count - 1) {
    resource = cut_resource(resource, resource->r_start + count);
  }

  return resource;
}

resource_t *rman_allocate_resource(rman_t *rm, rm_res_t start, rm_res_t end,
                                   rm_res_t count) {
  SCOPED_MTX_LOCK(&rm->mtx);
  resource_t *res = kmalloc(M_DEV, sizeof(resource_t), M_ZERO);

  resource_t *resource = find_resource(rm, start, end, count);
  if (resource == NULL) {
    return NULL;
  }

  resource = split_resource(resource, start, end, count);
  resource->r_flags |= RF_ALLOCATED;
  res->r_start = resource->r_start;
  res->r_end = resource->r_end;

  // TODO alignment

  rm->start += count;

  return res;
}

static void rman_init(rman_t *rm) {
  mtx_init(&rm->mtx, MTX_DEF);
  LIST_INIT(&rm->resources);

  // TODO so maybe we don't need to store start and end in rman_t?
  resource_t *whole_space = kmalloc(M_DEV, sizeof(resource_t), M_ZERO);

  whole_space->r_start = rm->start;
  whole_space->r_end = rm->end;

  LIST_INSERT_HEAD(&rm->resources, whole_space, resources);
}

void rman_create(rman_t *rm, rm_res_t start, rm_res_t end) {
  rm->start = start;
  rm->end = end;

  rman_init(rm);
}
