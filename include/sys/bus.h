#ifndef _SYS_BUS_H_
#define _SYS_BUS_H_

#include <machine/bus_defs.h>
#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/rman.h>

typedef struct bus_methods bus_methods_t;
typedef struct bus_driver bus_driver_t;
typedef struct intr_handler intr_handler_t;
typedef SLIST_HEAD(, resource) res_slist_t;

typedef struct resource_list_entry {
  SLIST_ENTRY(resource_list_entry) link;
  resource_t *res;   /* the actual resource when allocated */
  res_type_t type;   /* type argument to alloc_resource */
  int rid;           /* resource identifier */
  rman_addr_t start; /* start of resource range */
  size_t count;      /* number of bytes */
} resource_list_entry_t;

typedef SLIST_HEAD(, resource_list_entry) resource_list_t;

#define RESOURCE_LIST_OF(dev) ((resource_list_t *)(dev)->instance)

/*! \brief Initialize a resource list. */
void resource_list_init(resource_list_t *rl);

/*! \brief Reclaim memory used by a resouce list. */
void resource_list_fini(resource_list_t *rl);

/*! \brief Add a resource entry to resource list. */
void resource_list_add(resource_list_t *rl, res_type_t type, int rid,
                       rman_addr_t start, size_t count);

#define resource_list_add_irq(rl, rid, irq)                                    \
  resource_list_add((rl), RT_IRQ, (rid), (irq), 1)

/*! \brief Find a resource entry by type and rid. */
resource_list_entry_t *resource_list_find(resource_list_t *rl, res_type_t type,
                                          int rid);

/*! \brief Allocate a resource based on resource list entry. */
resource_t *resource_list_alloc(resource_list_t *rl, rman_t *rman,
                                res_type_t type, int rid, res_flags_t flags);

/*! \brief Release an allocated resource. */
void resource_list_release(resource_list_t *rl, res_type_t type, int rid,
                           resource_t *res);

/* `bus space` describes a method to access hardware resources mapped at some
 * address. We make no distinction between different kinds of physical address
 * spaces. Same hardware resource can be accessed in many different ways
 * depending on which bus it was attached to (e.g. I/O ports vs. MMIO) */
struct bus_space {
  /* mapping */
  int (*bs_map)(bus_addr_t, bus_size_t, bus_space_handle_t *);

  /* read (single) */
  uint8_t (*bs_read_1)(bus_space_handle_t, bus_size_t);
  uint16_t (*bs_read_2)(bus_space_handle_t, bus_size_t);
  uint32_t (*bs_read_4)(bus_space_handle_t, bus_size_t);

  /* write (single) */
  void (*bs_write_1)(bus_space_handle_t, bus_size_t, uint8_t);
  void (*bs_write_2)(bus_space_handle_t, bus_size_t, uint16_t);
  void (*bs_write_4)(bus_space_handle_t, bus_size_t, uint32_t);

  /* read region */
  void (*bs_read_region_1)(bus_space_handle_t, bus_size_t, uint8_t *,
                           bus_size_t);
  void (*bs_read_region_2)(bus_space_handle_t, bus_size_t, uint16_t *,
                           bus_size_t);
  void (*bs_read_region_4)(bus_space_handle_t, bus_size_t, uint32_t *,
                           bus_size_t);

  /* write region */
  void (*bs_write_region_1)(bus_space_handle_t, bus_size_t, const uint8_t *,
                            bus_size_t);
  void (*bs_write_region_2)(bus_space_handle_t, bus_size_t, const uint16_t *,
                            bus_size_t);
  void (*bs_write_region_4)(bus_space_handle_t, bus_size_t, const uint32_t *,
                            bus_size_t);
};

int generic_bs_map(bus_addr_t addr, bus_size_t size,
                   bus_space_handle_t *handle_p);

uint8_t generic_bs_read_1(bus_space_handle_t handle, bus_size_t offset);
uint16_t generic_bs_read_2(bus_space_handle_t handle, bus_size_t offset);
uint32_t generic_bs_read_4(bus_space_handle_t handle, bus_size_t offset);

void generic_bs_write_1(bus_space_handle_t handle, bus_size_t offset,
                        uint8_t value);
void generic_bs_write_2(bus_space_handle_t handle, bus_size_t offset,
                        uint16_t value);
void generic_bs_write_4(bus_space_handle_t handle, bus_size_t offset,
                        uint32_t value);

void generic_bs_read_region_1(bus_space_handle_t handle, bus_size_t offset,
                              uint8_t *dst, bus_size_t count);
void generic_bs_read_region_2(bus_space_handle_t handle, bus_size_t offset,
                              uint16_t *dst, bus_size_t count);
void generic_bs_read_region_4(bus_space_handle_t handle, bus_size_t offset,
                              uint32_t *dst, bus_size_t count);

void generic_bs_write_region_1(bus_space_handle_t handle, bus_size_t offset,
                               const uint8_t *src, bus_size_t count);
void generic_bs_write_region_2(bus_space_handle_t handle, bus_size_t offset,
                               const uint16_t *src, bus_size_t count);
void generic_bs_write_region_4(bus_space_handle_t handle, bus_size_t offset,
                               const uint32_t *src, bus_size_t count);

extern bus_space_t *generic_bus_space;

#define BUS_SPACE_DECLARE(name) extern bus_space_t name[1]

#define __bs_func(t, op, sz)                                                   \
  (*(t)->__CONCAT(__CONCAT(__CONCAT(bs_, op), _), sz))

#define __bs_read(t, h, o, sz) __bs_func((t), read, sz)((h), (o))
#define bus_space_read_1(t, h, o) __bs_read(t, h, o, 1)
#define bus_space_read_2(t, h, o) __bs_read(t, h, o, 2)
#define bus_space_read_4(t, h, o) __bs_read(t, h, o, 4)

#define __bs_write(t, h, o, v, sz) __bs_func((t), write, sz)((h), (o), (v))
#define bus_space_write_1(t, h, o, v) __bs_write(t, h, o, v, 1)
#define bus_space_write_2(t, h, o, v) __bs_write(t, h, o, v, 2)
#define bus_space_write_4(t, h, o, v) __bs_write(t, h, o, v, 4)

#define __bs_read_region(t, h, o, dst, cnt, sz)                                \
  __bs_func((t), read_region, sz)((h), (o), (dst), (cnt))
#define bus_space_read_region_1(t, h, o, dst, cnt)                             \
  __bs_read_region(t, h, o, dst, cnt, 1)
#define bus_space_read_region_2(t, h, o, dst, cnt)                             \
  __bs_read_region(t, h, o, dst, cnt, 2)
#define bus_space_read_region_4(t, h, o, dst, cnt)                             \
  __bs_read_region(t, h, o, dst, cnt, 4)

#define __bs_write_region(t, h, o, src, cnt, sz)                               \
  __bs_func((t), write_region, sz)((h), (o), (src), (cnt))
#define bus_space_write_region_1(t, h, o, src, cnt)                            \
  __bs_write_region(t, h, o, src, cnt, 1)
#define bus_space_write_region_2(t, h, o, src, cnt)                            \
  __bs_write_region(t, h, o, src, cnt, 2)
#define bus_space_write_region_4(t, h, o, src, cnt)                            \
  __bs_write_region(t, h, o, src, cnt, 4)

/* Simplified versions of the above, which take only a pointer to resource. */
#define __bus_read(r, o, sz)                                                   \
  __bs_read((r)->r_bus_tag, (r)->r_bus_handle, (o), sz)
#define bus_read_1(r, o) __bus_read(r, o, 1)
#define bus_read_2(r, o) __bus_read(r, o, 2)
#define bus_read_4(r, o) __bus_read(r, o, 4)

#define __bus_write(r, o, v, sz)                                               \
  __bs_write((r)->r_bus_tag, (r)->r_bus_handle, (o), (v), sz)
#define bus_write_1(r, o, v) __bus_write(r, o, v, 1)
#define bus_write_2(r, o, v) __bus_write(r, o, v, 2)
#define bus_write_4(r, o, v) __bus_write(r, o, v, 4)

#define __bus_read_region(r, o, dst, cnt, sz)                                  \
  __bs_write((r)->r_bus_tag, (r)->r_bus_handle, (o), (dst), (cnt), sz)
#define bus_read_region_1(r, o, dst, cnt) __bus_read_region(r, o, dst, cnt, 1)
#define bus_read_region_2(r, o, dst, cnt) __bus_read_region(r, o, dst, cnt, 2)
#define bus_read_region_4(r, o, dst, cnt) __bus_read_region(r, o, dst, cnt, 4)

#define __bus_write_region(r, o, src, cnt, sz)                                 \
  __bs_write_region((r)->r_bus_tag, (r)->r_bus_handle, (o), (src), (cnt), sz)
#define bus_write_region_1(r, o, src, cnt) __bus_write_region(r, o, src, cnt, 1)
#define bus_write_region_2(r, o, src, cnt) __bus_write_region(r, o, src, cnt, 2)
#define bus_write_region_4(r, o, src, cnt) __bus_write_region(r, o, src, cnt, 4)

#define bus_space_map(t, a, s, hp) (*(t)->bs_map)((a), (s), (hp))

struct bus_methods {
  void (*intr_setup)(device_t *dev, resource_t *irq, ih_filter_t *filter,
                     ih_service_t *service, void *arg, const char *name);
  void (*intr_teardown)(device_t *dev, resource_t *irq);
  resource_t *(*alloc_resource)(device_t *dev, res_type_t type, int rid,
                                rman_addr_t start, rman_addr_t end, size_t size,
                                res_flags_t flags);
  void (*release_resource)(device_t *dev, res_type_t type, int rid,
                           resource_t *r);
  int (*activate_resource)(device_t *dev, res_type_t type, int rid,
                           resource_t *r);
};

struct bus_driver {
  driver_t driver;
  bus_methods_t bus;
};

#define BUS_DRIVER(dev) ((bus_driver_t *)((dev)->parent->driver))

static inline void bus_intr_setup(device_t *dev, resource_t *irq,
                                  ih_filter_t *filter, ih_service_t *service,
                                  void *arg, const char *name) {
  BUS_DRIVER(dev)->bus.intr_setup(dev, irq, filter, service, arg, name);
}

static inline void bus_intr_teardown(device_t *dev, resource_t *irq) {
  BUS_DRIVER(dev)->bus.intr_teardown(dev, irq);
}

/*! \brief Allocates a resource of type \a type and size \a size between
 * \a start and \a end for a device \a dev.
 *
 * Should be called inside device's \fn attach function.
 *
 * \param dev device which needs resource
 * \param type resource type RT_* defined in rman.h
 * \param rid resource identifier as in \a resource_t structure
 * \param start/end - range of the addresses from which the resource will be
 * allocated
 * \param size the size of the resource
 * \param flags RF_* flags defined in rman.h
 */
static inline resource_t *bus_alloc_resource(device_t *dev, res_type_t type,
                                             int rid, rman_addr_t start,
                                             rman_addr_t end, size_t size,
                                             res_flags_t flags) {
  return BUS_DRIVER(dev)->bus.alloc_resource(dev, type, rid, start, end, size,
                                             flags);
}

/*! \brief Allocates resource for a device.
 *
 * \sa bus_resource_alloc with resource placement in memory
 * chosen by the parent bus.
 */
static inline resource_t *bus_alloc_resource_anywhere(device_t *dev,
                                                      res_type_t type, int rid,
                                                      size_t size,
                                                      res_flags_t flags) {
  return BUS_DRIVER(dev)->bus.alloc_resource(dev, type, rid, 0, RMAN_ADDR_MAX,
                                             size, flags);
}

/*! \brief Allocates resource for a device.
 *
 * Basically the same as \sa bus_alloc_resource_anywhere, but resource
 * has to be identifiable by parent bus driver by \param rid.
 */
static inline resource_t *bus_alloc_resource_any(device_t *dev, res_type_t type,
                                                 int rid, res_flags_t flags) {

  return BUS_DRIVER(dev)->bus.alloc_resource(dev, type, rid, 0, RMAN_ADDR_MAX,
                                             RMAN_SIZE_MAX, flags);
}

/*! \brief Activates resource for a device.
 *
 * This is a wrapper that calls bus method `activate_resource`.
 *
 * It performs common tasks like: check if resource has been already activated,
 * mark resource as activated if the method returned success.
 */
int bus_activate_resource(device_t *dev, res_type_t type, int rid,
                          resource_t *r);

static inline void bus_release_resource(device_t *dev, res_type_t type, int rid,
                                        resource_t *r) {
  BUS_DRIVER(dev)->bus.release_resource(dev, type, rid, r);
}

int bus_generic_probe(device_t *bus);

#endif /* !_SYS_BUS_H_ */
