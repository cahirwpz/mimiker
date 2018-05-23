#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include "bus.h"

typedef size_t rman_addr;

typedef struct rman rman_t;
typedef struct rman_block rman_block_t;

struct rman_block {
  rman_addr start;
  rman_addr end;
  bool is_allocated;
  LIST_ENTRY(rman_block) blocks;
};

struct rman {
  rman_addr start;
  rman_addr end;
  mtx_t mtx;
  LIST_HEAD(, rman_block) blocks;
};

void rman_init(rman_t *rm);

void rman_allocate_resource(resource_t *res, rman_t *rm, rman_addr start,
                            rman_addr end, rman_addr count);

inline void rman_allocate_resource_any(resource_t *res, rman_t *rm,
                                       rman_addr count) {
  rman_allocate_resource(res, rm, 0, (rman_addr)~0, count);
}

#endif /* _SYS_RMAN_H_ */
