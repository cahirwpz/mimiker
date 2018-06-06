#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <common.h>
#include <device.h>
#include <mutex.h>

typedef long unsigned rman_addr_t;

typedef struct rman rman_t;
typedef struct rman_block rman_block_t;
typedef struct resource resource_t;
typedef struct bus_space bus_space_t;

/* TODO update this comment
 * `resource` describes a range of addresses where a resource is mapped within
 * resource. A driver will use addresses from `r_start` to `r_end` and
 * `r_bus_space` routines to access hardware resource, so that the actual
 * driver code is not tied to way handled device is attached to the system. */
#define RT_UNKNOWN 0
#define RT_MEMORY 1
#define RT_IOPORTS 2

#define RF_NONE 0
/* According to PCI specification prefetchable bit is CLEAR when memory mapped
 * resource contains locations with read side-effects or locations in which the
 * device does not tolerate write merging. */
#define RF_EXCLUSIVE 0
#define RF_PREFETCHABLE 1
#define RF_SHARED 2 // TODO
#define RF_ALLOCATED 4
#define RF_NEEDS_ACTIVATION 8

/* alingment is always power of two, and log2() of it is stored in rf_flags */
#define RF_ALIGNMENT_SHIFT 10
#define RF_ALIGNMENT_MASK (0x003F << RF_ALIGNMENT_SHIFT)
#define RF_GET_ALIGNMENT(x)                                                    \
  (1 << (((x)&RF_ALIGNMENT_MASK) >> RF_ALIGNMENT_SHIFT))

struct resource {
  bus_space_t *r_bus_space; /* bus space accessor descriptor */
  void *r_owner;            /* pointer to device that owns this resource */
  rman_addr_t r_start;      /* first physical address of the resource */
  rman_addr_t r_end; /* last (inclusive) physical address of the resource */
  unsigned r_type;
  unsigned r_flags;
  int r_id; /* (optional) resource identifier */
  LIST_ENTRY(resource) r_resources;
};

struct rman {
  rman_addr_t rm_start;
  rman_addr_t rm_end;
  mtx_t rm_mtx;
  LIST_HEAD(, resource) rm_resources;
};

// returns null if unable to allocate
resource_t *rman_allocate_resource(rman_t *rm, rman_addr_t start,
                                   rman_addr_t end, rman_addr_t count,
                                   unsigned flags);

static inline resource_t *
rman_allocate_resource_anywhere(rman_t *rm, rman_addr_t count, unsigned flags) {
  return rman_allocate_resource(rm, 0, (rman_addr_t)~0, count, flags);
}

void rman_create(rman_t *rm, rman_addr_t start, rman_addr_t end);

static inline void rman_create_from_resource(rman_t *rm, resource_t *res) {
  rman_create(rm, res->r_start, res->r_end);
}

unsigned rman_make_alignment_flags(rman_addr_t align);

#endif /* _SYS_RMAN_H_ */
