#include <rman.h>
#include <stdc.h>
#include <pool.h>

static POOL_DEFINE(P_RMAN, "rman", sizeof(resource_t), NULL, NULL);

static resource_t *find_resource(rman_t *rm, rman_addr_t start, rman_addr_t end,
                                 size_t count, size_t alignment,
                                 unsigned flags) {
  resource_t *resource;
  LIST_FOREACH(resource, &rm->rm_resources, r_resources) {
    if ((resource->r_flags & RF_ALLOCATED) &&
        !((resource->r_flags & RF_SHARED) && (flags & RF_SHARED)))
      continue;

    if ((start > resource->r_end) || (end < resource->r_start))
      continue;

    /* Calculate common part and check if is big enough. */
    rman_addr_t s = align(max(start, resource->r_start), alignment);
    rman_addr_t e = min(end, resource->r_end);
    /* size of address range after alignment */
    rman_addr_t len = e - s + 1;

    /* When trying to use existing resource, size should be the same. */
    if ((flags & RF_SHARED) &&
        ((rman_addr_t)count != resource->r_end - resource->r_start + 1))
      continue;

    if (len >= (rman_addr_t)count)
      return resource;
  }

  return NULL;
}

/* !\brief cut the resource into two
 *
 * Divide resource into two `where` means start of right resource function
 * returns pointer to new (right) resource.
 */
static resource_t *cut_resource(resource_t *res, rman_addr_t where) {
  assert(where > res->r_start);
  assert(where < res->r_end);

  resource_t *left_res = res;

  resource_t *right_res = pool_alloc(P_RMAN, 0);
  memset(right_res, 0, sizeof(resource_t));

  LIST_INSERT_AFTER(left_res, right_res, r_resources);

  right_res->r_end = left_res->r_end;
  right_res->r_start = where;
  right_res->r_type = res->r_type;
  left_res->r_end = where - 1;

  return right_res;
}

/* !\brief Extract resource with given addres range from bigger resource.
 *
 * Maybe split resource into two or three in order to recover space before and
 * after allocation.
 */
static resource_t *split_resource(resource_t *res, rman_addr_t start,
                                  rman_addr_t end, size_t count) {
  if (res->r_start < start)
    res = cut_resource(res, start);

  if (res->r_end > res->r_start + (rman_addr_t)count - 1)
    cut_resource(res, res->r_start + count);

  return res;
}

resource_t *rman_allocate_resource(rman_t *rm, rman_addr_t start,
                                   rman_addr_t end, size_t count, size_t align,
                                   unsigned flags) {
  SCOPED_MTX_LOCK(&rm->rm_mtx);

  align = min(align, sizeof(void *));

  resource_t *resource = find_resource(rm, start, end, count, align, flags);
  if (resource == NULL)
    return NULL;

  resource = split_resource(resource, align(start, align), end, count);
  resource->r_type = rm->type;
  resource->r_align = align;
  resource->r_flags = flags;
  resource->r_flags |= RF_ALLOCATED;

  return resource;
}

void rman_create(rman_t *rm, rman_addr_t start, rman_addr_t end,
                 resource_type_t type) {
  rm->rm_start = start;
  rm->rm_end = end;
  rm->type = type;

  mtx_init(&rm->rm_mtx, MTX_DEF);
  LIST_INIT(&rm->rm_resources);

  resource_t *whole_space = pool_alloc(P_RMAN, 0);
  memset(whole_space, 0, sizeof(resource_t));

  whole_space->r_start = rm->rm_start;
  whole_space->r_end = rm->rm_end;
  whole_space->r_type = type;

  LIST_INSERT_HEAD(&rm->rm_resources, whole_space, r_resources);
}

void rman_create_from_resource(rman_t *rm, resource_t *res) {
  assert(res->r_flags & RF_ALLOCATED);
  rman_create(rm, res->r_start, res->r_end, res->r_type);
}
