#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <common.h>
#include <mutex.h>
#include <queue.h>

typedef unsigned rman_addr_t;

typedef struct rman rman_t;
typedef struct resource resource_t;
typedef struct bus_space bus_space_t;
typedef LIST_HEAD(, resource) resource_list_t;

/* We need to have some PCI-ISA bridge driver in place. */

#define RT_UNKNOWN 0
#define RT_MEMORY 1
#define RT_IOPORTS 2
#define RT_ISA_F 4 /* TODO: remove after ISA-bridge driver is implemented */

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
  unsigned r_type;   /* one of RT_* values (possibly or'ed with RT_ISA_F) */
  unsigned r_flags;  /* or'ed RF_* values */
  size_t r_align;    /* alignment requirements for starting physical address */
  int r_id;          /* (optional) resource identifier */
  LIST_ENTRY(resource) r_resources; /* link on resource manager list */
  LIST_ENTRY(resource) r_device; /* link on resources list assigned to device */
};

struct rman {
  mtx_t rm_mtx;                 /* protects all fields of resource manager */
  rman_addr_t rm_start;         /* first physical address */
  rman_addr_t rm_end;           /* last physical adress */
  resource_list_t rm_resources; /* all managed resources */
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
