#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <common.h>
#include <mutex.h>
#include <queue.h>

typedef unsigned rman_addr_t;

typedef struct rman rman_t;
typedef struct resource resource_t;
typedef struct bus_space bus_space_t;

/* We need to have some PCI-ISA bridge driver in place. */

#define RT_UNKNOWN 0
#define RT_MEMORY 1
#define RT_IOPORTS 2
#define RT_ISA_F 4 /* temporary hack */

#define RF_NONE 0
/* According to PCI specification prefetchable bit is CLEAR when memory mapped
 * resource contains locations with read side-effects or locations in which the
 * device does not tolerate write merging. */
#define RF_PREFETCHABLE 1
#define RF_SHARED 2
#define RF_ALLOCATED 4

struct resource {
  bus_space_t *r_bus_space; /* bus space accessor descriptor */
  void *r_owner;            /* pointer to device that owns this resource */
  rman_addr_t r_start;      /* first physical address of the resource */
  rman_addr_t r_end; /* last (inclusive) physical address of the resource */
  unsigned r_type;
  unsigned r_flags;
  size_t r_align;
  int r_id; /* (optional) resource identifier */
  LIST_ENTRY(resource) r_resources;
  LIST_ENTRY(resource) r_device;
};

struct rman {
  rman_addr_t rm_start;
  rman_addr_t rm_end;
  mtx_t rm_mtx;
  LIST_HEAD(, resource) rm_resources;
};

/* returns null if unable to allocate */
resource_t *rman_allocate_resource(rman_t *rm, rman_addr_t start,
                                   rman_addr_t end, size_t count, size_t align,
                                   unsigned flags);

static inline resource_t *rman_allocate_resource_anywhere(rman_t *rm,
                                                          size_t count,
                                                          size_t align,
                                                          unsigned flags) {
  return rman_allocate_resource(rm, 0, (rman_addr_t)~0, count, align, flags);
}

void rman_create(rman_t *rm, rman_addr_t start, rman_addr_t end);

static inline void rman_create_from_resource(rman_t *rm, resource_t *res) {
  rman_create(rm, res->r_start, res->r_end);
}

void rman_init(void);

#endif /* _SYS_RMAN_H_ */
