#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include "bus.h"

typedef intptr_t rman_addr; // TODO resource uses other datatype

typedef struct rman rman_t;
typedef struct rman_block rman_block_t;

struct rman {
  rman_addr start;
  rman_addr end;
  mtx_t mtx;
  LIST_HEAD(, resource) resources;
};

void rman_init(rman_t *rm);

// return null if unable to allocate
resource_t *rman_allocate_resource(rman_t *rm, rman_addr start, rman_addr end,
                                   rman_addr count);

inline resource_t *rman_allocate_resource_any(rman_t *rm, rman_addr count) {
  return rman_allocate_resource(rm, 0, (rman_addr)~0, count);
}

#endif /* _SYS_RMAN_H_ */
