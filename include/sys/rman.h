#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <sys/cdefs.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <machine/bus_defs.h>

#define RMAN_ADDR_MAX UINTPTR_MAX
#define RMAN_SIZE_MAX UINTPTR_MAX

typedef uintptr_t rman_addr_t;
typedef struct rman rman_t;
typedef struct resource resource_t;
typedef struct rman_resource rman_resource_t;
typedef struct device device_t;
typedef TAILQ_HEAD(rman_res_list, rman_resource) rman_res_list_t;

typedef enum {
  RF_RESERVED = 1,
  RF_ACTIVE = 2,
  /* According to PCI specification prefetchable bit is CLEAR when memory mapped
   * resource contains locations with read side-effects or locations in which
   * the device does not tolerate write merging. */
  RF_PREFETCHABLE = 4,
} res_flags_t;

/*! \brief Calculate resource size. */
bus_size_t resource_size(resource_t *r);

/*! \brief Return resource start address within the rman range. */
rman_addr_t resource_start(resource_t *r);

/*! \brief Check whether a resource is active. */
bool resource_active(resource_t *r);

#define RESOURCE_DECLARE(name) extern resource_t name[1]

struct rman {
  mtx_t rm_lock;                /* protects all fields of resource manager */
  const char *rm_name;          /* description of the resource manager */
  rman_res_list_t rm_resources; /* resources managed by this resource manager */
};

/* !\brief Reserve a rman resource within given rman.
 *
 * Looks up a region of size `count` between `start` and `end` address.
 * Assigned starting address will be aligned to `alignment` which must be
 * power of 2.
 *
 * \returns NULL if could not allocate a resource
 */
rman_resource_t *rman_reserve_resource(rman_t *rm, rman_addr_t start,
                                       rman_addr_t end, size_t count,
                                       size_t alignment, res_flags_t flags);

/*! \brief Removes a resource from its resource manager and releases memory. */
void rman_release_resource(rman_resource_t *r);

/*! \brief Marks resource as ready to be used with bus_space interface. */
void rman_activate_resource(rman_resource_t *r);

/*! \brief Marks resource as deactivated. */
void rman_deactivate_resource(rman_resource_t *r);

/* !\brief Initializes resource manager for further use. */
void rman_init(rman_t *rm, const char *name);

/* !\brief Initializes resource manager based on supplied resource. */
void rman_init_from_resource(rman_t *rm, const char *name, resource_t *r);

/* !\brief Adds a new region to be managed by a resource manager. */
void rman_manage_region(rman_t *rm, rman_addr_t start, size_t size);

/* !\brief Destroy resource manager and free its memory resources. */
void rman_fini(rman_t *rm);

#endif /* !_SYS_RMAN_H_ */
