#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <common.h>
#include <mutex.h>
#include <queue.h>

/* TODO: remove RT_ISA after ISA-bridge driver is implemented */
typedef enum { RT_UNKNOWN, RT_IOPORTS, RT_MEMORY, RT_ISA } res_type_t;
typedef uintptr_t rman_addr_t;
#define RMAN_ADDR_MAX UINTPTR_MAX

typedef struct rman rman_t;
typedef struct resource resource_t;
typedef struct device device_t;
typedef struct bus_space bus_space_t;
typedef TAILQ_HEAD(, resource) res_list_t;

typedef enum {
  RF_NONE = 0,
  /* According to PCI specification prefetchable bit is CLEAR when memory mapped
   * resource contains locations with read side-effects or locations in which
   * the device does not tolerate write merging. */
  RF_PREFETCHABLE = 1,
  RF_SHARED = 2, /* XXX: this flag does nothing right now */
  RF_ACTIVATED = 4,
} res_flags_t;

struct resource {
  bus_space_t *r_bus_space;       /* bus space accessor descriptor */
  device_t *r_owner;              /* device that owns this resource */
  rman_addr_t r_start;            /* first physical address of the resource */
  rman_addr_t r_end;              /* last (inclusive) physical address */
  res_type_t r_type;              /* one of RT_* */
  res_flags_t r_flags;            /* or'ed RF_* values */
  int r_id;                       /* (optional) resource identifier */
  TAILQ_ENTRY(resource) r_link;   /* link on resource manager list */
  TAILQ_ENTRY(resource) r_device; /* resources assigned to `r_owner` */
};

#define RESOURCE_DECLARE(name) extern resource_t name[1]

struct rman {
  mtx_t rm_lock;           /* protects all fields of resource manager */
  rman_addr_t rm_start;    /* first physical address */
  rman_addr_t rm_end;      /* last physical adress */
  res_list_t rm_resources; /* all managed resources */
  res_type_t rm_type;      /* type of managed resources */
};

/* !\brief Allocate resource within given rman.
 *
 * Looks up a region of size `count` between `start` and `end` address.
 * Assigned starting address will be aligned to `alignment` which must be
 * power of 2. Resource will be marked as owned by `dev` device.
 *
 * \returns NULL if could not allocate a resource
 */
resource_t *rman_alloc_resource(rman_t *rm, rman_addr_t start, rman_addr_t end,
                                size_t count, size_t bound, res_flags_t flags,
                                device_t *dev);

/* !\brief Create and initialize new rman.
 *
 * \param type specifies type of resources managed by this rman.
 */
void rman_create(rman_t *rm, rman_addr_t start, rman_addr_t end,
                 res_type_t type);

/* !\brief Consume resource for exclusive use of new rman. */
void rman_create_from_resource(rman_t *rm, resource_t *res);

#endif /* _SYS_RMAN_H_ */
