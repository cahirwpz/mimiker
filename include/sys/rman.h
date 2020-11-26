#ifndef _SYS_RMAN_H_
#define _SYS_RMAN_H_

#include <sys/cdefs.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <machine/bus_defs.h>

typedef enum { RT_UNKNOWN, RT_IOPORTS, RT_MEMORY, RT_IRQ } res_type_t;
typedef uintptr_t rman_addr_t;
#define RMAN_ADDR_MAX UINTPTR_MAX
#define RMAN_SIZE_MAX UINTPTR_MAX

typedef struct intr_handler intr_handler_t;
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
  RF_SHAREABLE = 2, /* XXX: this flag does nothing right now */
  RF_ACTIVE = 4,
} res_flags_t;

/* XXX: since a resource should be created only as a result of a call to
 * `rman_alloc_resource` perhaps we could make the definition private? */
struct resource {
  bus_space_tag_t r_bus_tag;       /* bus space methods */
  bus_space_handle_t r_bus_handle; /* bus space base address */
  rman_t *r_rman;                  /* resource manager of this resource */
  rman_addr_t r_start;             /* first physical address of the resource */
  rman_addr_t r_end;               /* last (inclusive) physical address */
  /* auxiliary data associated with a resource */
  union {
    intr_handler_t *r_handler;
  };
  /* TODO: remove r_type from this structure as r_rman->r_type contains the same
   * information. See `rman_alloc_resource` for setting r_type of a resource. */
  res_type_t r_type;            /* one of RT_* */
  res_flags_t r_flags;          /* or'ed RF_* values */
  TAILQ_ENTRY(resource) r_link; /* link on resource manager list */
};

#define RESOURCE_DECLARE(name) extern resource_t name[1]

struct rman {
  mtx_t rm_lock;           /* protects all fields of resource manager */
  const char *rm_name;     /* description of the resource manager */
  rman_addr_t rm_start;    /* first physical address */
  rman_addr_t rm_end;      /* last physical adress */
  res_list_t rm_resources; /* all managed resources */
  res_type_t rm_type;      /* type of managed resources */
};

/* !\brief Allocate resource within given rman.
 *
 * Looks up a region of size `count` between `start` and `end` address.
 * Assigned starting address will be aligned to `alignment` which must be
 * power of 2.
 *
 * \returns NULL if could not allocate a resource
 */
resource_t *rman_alloc_resource(rman_t *rm, rman_addr_t start, rman_addr_t end,
                                size_t count, size_t bound, res_flags_t flags);

/*! \brief Removes a resource from its resource manager and releases memory. */
void rman_release_resource(resource_t *r);
/*! \brief Mark resource as ready to be used with bus_space interface. */
void rman_activate_resource(resource_t *r);

/*! \brief Calculate resource size. */
static inline bus_size_t rman_get_size(resource_t *r) {
  return r->r_end - r->r_start + 1;
}

/* !\brief Initializes resource manager for further use. */
void rman_init(rman_t *rm, const char *name, rman_addr_t start, rman_addr_t end,
               res_type_t type);

#endif /* !_SYS_RMAN_H_ */
