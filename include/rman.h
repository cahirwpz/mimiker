#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <common.h>
#include <device.h>

typedef long unsigned rman_addr;

typedef struct rman rman_t;
typedef struct rman_block rman_block_t;
typedef struct resource resource_t;
typedef struct bus_space bus_space_t;

/* `resource` describes a range of addresses where a resource is mapped within
 * given bus space. A driver will use addresses from `r_start` to `r_end` and
 * `r_bus_space` routines to access hardware resource, so that the actual
 * driver code is not tied to way handled device is attached to the system. */

#define RT_UNKNOWN 0
#define RT_MEMORY 1
#define RT_IOPORTS 2

#define RF_NONE 0
/* According to PCI specification prefetchable bit is CLEAR when memory mapped
 * resource contains locations with read side-effects or locations in which the
 * device does not tolerate write merging. */
#define RF_PREFETCHABLE 1
#define RF_SHARED 2 // TODO
#define RF_ALLOCATED 4

struct resource {
  bus_space_t *r_bus_space; /* bus space accessor descriptor */
  void *r_owner;            /* pointer to device that owns this resource */
  rman_addr r_start;        /* first physical address of the resource */
  rman_addr r_end; /* last (inclusive) physical address of the resource */
  unsigned r_type;
  unsigned r_flags;
  int r_id; /* (optional) resource identifier */
  LIST_ENTRY(resource) resources;
};

struct rman {
  rman_addr start;
  rman_addr end;
  mtx_t mtx;
  LIST_HEAD(, resource) resources;
};

// returns null if unable to allocate
resource_t *rman_allocate_resource(rman_t *rm, rman_addr start, rman_addr end,
                                   rman_addr count);

static inline resource_t *rman_allocate_resource_any(rman_t *rm) {
  return rman_allocate_resource(rm, 0, (rman_addr)~0, 1);
}

static inline resource_t *rman_allocate_resource_anywhere(rman_t *rm,
                                                          rman_addr count) {
  return rman_allocate_resource(rm, 0, (rman_addr)~0, count);
}

void rman_create(rman_t *rm, rman_addr start, rman_addr end);

static inline void rman_create_from_resource(rman_t *rm, resource_t *res) {
  rman_create(rm, res->r_start, res->r_end);
}

#endif /* _SYS_RMAN_H_ */
