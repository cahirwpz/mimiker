#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <common.h>
#include <mutex.h>
#include <queue.h>

/* TODO: remove RT_ISA after ISA-bridge driver is implemented */
typedef enum { RT_UNKNOWN, RT_IOPORTS, RT_MEMORY, RT_ISA } resource_type_t;
typedef intptr_t rman_addr_t;
#define RMAN_ADDR_MAX INTPTR_MAX

typedef struct rman rman_t;
typedef struct resource resource_t;
typedef struct bus_space bus_space_t;
typedef LIST_HEAD(, resource) resource_list_t;

#define RF_NONE 0
/* According to PCI specification prefetchable bit is CLEAR when memory mapped
 * resource contains locations with read side-effects or locations in which the
 * device does not tolerate write merging. */
#define RF_PREFETCHABLE 1
#define RF_SHARED 2
#define RF_ALLOCATED 4 /* For internal use. Every returned resource has it. */
#define RF_ACTIVATED 8

struct resource {
  bus_space_t *r_bus_space; /* bus space accessor descriptor */
  void *r_owner;            /* pointer to device that owns this resource */
  rman_addr_t r_start;      /* first physical address of the resource */
  rman_addr_t r_end; /* last (inclusive) physical address of the resource */
  resource_type_t r_type;
  unsigned r_flags; /* or'ed RF_* values */
  size_t r_align;   /* alignment requirements for starting physical address */
  int r_id;         /* (optional) resource identifier */
  LIST_ENTRY(resource) r_resources; /* link on resource manager list */
  LIST_ENTRY(resource) r_device; /* link on resources list assigned to device */
};

struct rman {
  mtx_t rm_mtx;                 /* protects all fields of resource manager */
  rman_addr_t rm_start;         /* first physical address */
  rman_addr_t rm_end;           /* last physical adress */
  resource_list_t rm_resources; /* all managed resources */
  resource_type_t type;         /* type of managed resources */
};

/* !\brief Allocate resource within given rman.
 *
 * Returns NULL if unable to allocate.
 */
resource_t *rman_allocate_resource(rman_t *rm, rman_addr_t start,
                                   rman_addr_t end, size_t count, size_t align,
                                   unsigned flags);

/* !\brief Create and initialize new rman.
 *
 * \param type specifies type of resources managed by this rman.
 */
void rman_create(rman_t *rm, rman_addr_t start, rman_addr_t end,
                 resource_type_t type);

/* !\brief Consume resource for exclusive use of new rman.
 */
static inline void rman_create_from_resource(rman_t *rm, resource_t *res) {
  rman_create(rm, res->r_start, res->r_end, res->r_type);
}

void rman_init(void);

#endif /* _SYS_RMAN_H_ */
