#include <rman.h>

void rman_allocate_resource(resource_t *res, rman_t *rm, rman_addr start,
                            rman_addr end, rman_addr count) {
  SCOPED_MTX_LOCK(&rm->mtx);

  // TODO only these settings are supported for now
  assert(start == 0);
  assert(end == (rman_addr)~0);

  assert(rm->end - rm->start >= count);

  res->r_start = rm->start; // TODO alignment
  res->r_end = rm->start + count - 1;

  rm->start += count;
}


void rman_init(rman_t *rm) {
  mtx_init(&rm->mtx, MTX_DEF);
  LIST_INIT(&rm->blocks);
}