#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <sys/cdefs.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <machine/bus_defs.h>

#define RMAN_ADDR_MAX UINTPTR_MAX
#define RMAN_SIZE_MAX UINTPTR_MAX

typedef uintptr_t rman_addr_t;
typedef enum res_type res_type_t;
typedef struct rman rman_t;
typedef struct resource resource_t;
typedef struct range range_t;
typedef struct device device_t;
typedef TAILQ_HEAD(range_list, range) range_list_t;

typedef enum {
  RF_RESERVED = 1,
  RF_ACTIVE = 2,
  /* According to PCI specification prefetchable bit is CLEAR when memory mapped
   * range contains locations with read side-effects or locations in which the
   * device does not tolerate write merging. */
  RF_PREFETCHABLE = 4,
} rman_flags_t;

struct range {
  rman_t *rman;            /* manager of this range */
  rman_addr_t start;       /* first physical address of the range */
  rman_addr_t end;         /* last (inclusive) physical address */
  rman_flags_t flags;      /* or'ed RF_* values */
  TAILQ_ENTRY(range) link; /* link on range manager list */
};

struct rman {
  mtx_t rm_lock;          /* protects all fields of range manager */
  const char *rm_name;    /* description of the range manager */
  range_list_t rm_ranges; /* ranges managed by this range manager */
};

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

void resource_release(resource_t *r);

/*! \brief Marks range as ready to be used with bus_space interface. */
void rman_activate_range(range_t *r);

/*! \brief Marks range as deactivated. */
void rman_deactivate_range(range_t *r);

/* !\brief Initializes range manager for further use. */
void rman_init(rman_t *rm, const char *name);

/* !\brief Initializes range manager based on supplied resource. */
void rman_init_from_resource(rman_t *rm, const char *name, resource_t *r);

/* !\brief Adds a new region to be managed by a range manager. */
void rman_manage_region(rman_t *rm, rman_addr_t start, size_t size);

/* !\brief Destroy range manager. */
void rman_fini(rman_t *rm);

#endif /* !_SYS_RMAN_H_ */
