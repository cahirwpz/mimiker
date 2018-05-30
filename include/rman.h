#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include "bus.h"

typedef struct rman rman_t;
typedef struct rman_block rman_block_t;

struct rman {
  intptr_t start;
  intptr_t end;
  mtx_t mtx;
  LIST_HEAD(, resource) resources;
};

// returns null if unable to allocate
resource_t *rman_allocate_resource(rman_t *rm, intptr_t start, intptr_t end,
                                   intptr_t count);

inline resource_t *rman_allocate_resource_any(rman_t *rm) {
  return rman_allocate_resource(rm, 0, (intptr_t)~0, 1);
}

inline resource_t *rman_allocate_resource_anywhere(rman_t *rm, intptr_t count) {
  return rman_allocate_resource(rm, 0, (intptr_t)~0, count);
}

void rman_create(rman_t *rm, intptr_t start, intptr_t end);

inline void rman_create_from_resource(rman_t *rm, resource_t *res) {
  rman_create(rm, res->r_start, res->r_end);
}


#endif /* _SYS_RMAN_H_ */
