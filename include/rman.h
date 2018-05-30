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

void rman_init(rman_t *rm);

// return null if unable to allocate
resource_t *rman_allocate_resource(rman_t *rm, intptr_t start, intptr_t end,
                                   intptr_t count);

inline resource_t *rman_allocate_resource_any(rman_t *rm, intptr_t count) {
  return rman_allocate_resource(rm, 0, (intptr_t)~0, count);
}

#endif /* _SYS_RMAN_H_ */
