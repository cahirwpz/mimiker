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
typedef struct range range_t;
typedef struct resource resource_t;
typedef struct device device_t;
typedef struct intr_handler intr_handler_t;
typedef TAILQ_HEAD(range_list, range) range_list_t;

typedef enum {
  RF_RESERVED = 1,
  RF_ACTIVE = 2,
  /* According to PCI specification prefetchable bit is CLEAR when memory mapped
   * range contains locations with read side-effects or locations in which the
   * device does not tolerate write merging. */
  RF_PREFETCHABLE = 4,
} rman_flags_t;

struct rman {
  mtx_t rm_lock;          /* protects all fields of range manager */
  const char *rm_name;    /* description of the range manager */
  range_list_t rm_ranges; /* ranges managed by this range manager */
};

typedef enum res_type {
  RT_IOPORTS,
  RT_MEMORY,
  RT_IRQ,
} res_type_t;

struct resource {
  SLIST_ENTRY(resource) r_link;
  range_t *r_range;  /* resource range expressed by (start, end) pair */
  res_type_t r_type; /* type, one of RT_* */
  int r_rid;         /* unique identifier */
  /* data specific to given resource type */
  union {
    /* interrupt resources */
    intr_handler_t *r_handler;

    /* memory and I/O port resources */
    struct {
      bus_space_tag_t r_bus_tag;       /* bus space methods */
      bus_space_handle_t r_bus_handle; /* bus space base address */
    };
  };
};

#define RESOURCE_DECLARE(name) extern resource_t name[1]

/*! \brief Calculate resource size. */
bus_size_t resource_size(resource_t *r);

/*! \brief Return resource start address within the rman range. */
rman_addr_t resource_start(resource_t *r);

/*! \brief Check whether a resource is active. */
bool resource_active(resource_t *r);

/* !\brief Reserve a resource with a range from given rman.
 *
 * Looks up a region of size `count` between `start` and `end` address.
 * Assigned starting address will be aligned to `alignment` which must be
 * power of 2. Initializes resource `type` and `rid`.
 *
 * \returns NULL if could not allocate a range
 */
resource_t *rman_reserve_resource(rman_t *rm, res_type_t, int rid,
                                  rman_addr_t start, rman_addr_t end,
                                  size_t count, size_t alignment,
                                  rman_flags_t flags);

/*! \brief Releases a resource and frees its memory. */
void resource_release(resource_t *r);

/*! \brief Marks resource as ready to be used with bus_space interface. */
void resource_activate(resource_t *r);

/*! \brief Marks resource as deactivated. */
void resource_deactivate(resource_t *r);

/* !\brief Initializes range manager for further use. */
void rman_init(rman_t *rm, const char *name);

/* !\brief Initializes range manager based on supplied resource. */
void rman_init_from_resource(rman_t *rm, const char *name, resource_t *r);

/* !\brief Adds a new region to be managed by a range manager. */
void rman_manage_region(rman_t *rm, rman_addr_t start, size_t size);

/* !\brief Destroy range manager. */
void rman_fini(rman_t *rm);

#endif /* !_SYS_RMAN_H_ */
