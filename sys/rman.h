#include "bus.h"

typedef size_t rman_addr;

struct rman {
  rman_addr start;
  rman_addr end;
  mtx_t mtx; // TODO maybe initialize this mutex
};

struct rman rman_iospace = {
  .start = 0x18000000, .end = 0x1bdfffff,
};

struct rman rman_memspace = {
  .start = 0x10000000, .end = 0x17ffffff,
};

void rman_allocate_resource(resource_t *res, struct rman *rm, rman_addr start,
                            rman_addr end, rman_addr count) {
  SCOPED_MTX_LOCK(&rm->mtx);

  // TODO only these settings are supported for now
  assert(start == 0);
  assert(end == (rman_addr)~0);

  assert(rm->end - rm->start >= count);

  res->r_start = rm->start;
  res->r_end = rm->start + count - 1;

  rm->start += count;
}
