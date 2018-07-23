#ifndef _SYS_BUS_H_
#define _SYS_BUS_H_

#include <_bus.h>
#include <device.h>
#include <rman.h>

typedef struct bus_methods bus_methods_t;
typedef struct bus_driver bus_driver_t;
typedef struct intr_handler intr_handler_t;

/* `bus space` describes a method to access hardware resources mapped at some
 * address. We make no distinction between different kinds of physical address
 * spaces. Same hardware resource can be accessed in many different ways
 * depending on which bus it was attached to (e.g. I/O ports vs. MMIO) */
struct bus_space {
  /* read (single) */
  uint8_t (*bs_read_1)(bus_addr_t, off_t);
  uint16_t (*bs_read_2)(bus_addr_t, off_t);
  uint32_t (*bs_read_4)(bus_addr_t, off_t);

  /* write (single) */
  void (*bs_write_1)(bus_addr_t, off_t, uint8_t);
  void (*bs_write_2)(bus_addr_t, off_t, uint16_t);
  void (*bs_write_4)(bus_addr_t, off_t, uint32_t);

  /* read region */
  void (*bs_read_region_1)(bus_addr_t, off_t, uint8_t *, size_t);

  /* write region */
  void (*bs_write_region_1)(bus_addr_t, off_t, const uint8_t *, size_t);
};

#define BUS_SPACE_DECLARE(name) extern bus_space_t name[1]

#define __bs_func(bs, op, sz)                                                  \
  (*(bs)->__CONCAT(__CONCAT(__CONCAT(bs_, op), _), sz))

#define __bs_read(bs, ba, o, sz) __bs_func((bs), read, sz)((ba), (o))
#define bus_space_read_1(bs, ba, o) __bs_read(bs, ba, o, 1)
#define bus_space_read_2(bs, ba, o) __bs_read(bs, ba, o, 2)
#define bus_space_read_4(bs, ba, o) __bs_read(bs, ba, o, 4)

#define __bs_write(bs, ba, o, v, sz) __bs_func((bs), write, sz)((ba), (o), (v))
#define bus_space_write_1(bs, ba, o) __bs_write(bs, ba, o, 1)
#define bus_space_write_2(bs, ba, o) __bs_write(bs, ba, o, 2)
#define bus_space_write_4(bs, ba, o) __bs_write(bs, ba, o, 4)

#define __bs_read_region(bs, ba, o, dst, cnt, sz)                              \
  __bs_func((bs), read_region, sz)((ba), (o), (dst), (cnt))
#define bus_space_read_region_1(bs, ba, o, dst, cnt)                           \
  __bs_read_region(bs, ba, o, dst, cnt, 1)

#define __bs_write_region(bs, ba, o, src, cnt, sz)                             \
  __bs_func((bs), write_region, sz)((ba), (o), (src), (cnt))
#define bus_space_write_region_1(bs, ba, o, src, cnt)                          \
  __bs_write_region(bs, ba, o, src, cnt, 1)

/* Simplified versions of the above, which take only a pointer to resource. */
#define __bus_read(r, o, sz)                                                   \
  __bs_read((r)->r_bus_space, (r)->r_bus_addr, (o) + (r)->r_start, sz)
#define bus_read_1(r, o) __bus_read(r, o, 1)
#define bus_read_2(r, o) __bus_read(r, o, 2)
#define bus_read_4(r, o) __bus_read(r, o, 4)

#define __bus_write(r, o, v, sz)                                               \
  __bs_write((r)->r_bus_space, (r)->r_bus_addr, (o) + (r)->r_start, (v), sz)
#define bus_write_1(r, o, v) __bus_write(r, o, v, 1)
#define bus_write_2(r, o, v) __bus_write(r, o, v, 2)
#define bus_write_4(r, o, v) __bus_write(r, o, v, 4)

#define __bus_read_region(r, o, dst, cnt, sz)                                  \
  __bs_write((r)->r_bus_space, (r)->r_bus_addr, (o) + (r)->r_start, (dst),     \
             (cnt), sz)
#define bus_read_region_1(r, o, dst, cnt) __bus_read_region(r, o, dst, cnt, 1)

#define __bus_write_region(r, o, src, cnt, sz)                                 \
  __bs_write_region((r)->r_bus_space, (r)->r_bus_addr, (o) + (r)->r_start,     \
                    (src), (cnt), sz)
#define bus_write_region_1(r, o, src, cnt) __bus_write_region(r, o, src, cnt, 1)

typedef void (*bus_intr_setup_t)(device_t *dev, unsigned num,
                                 intr_handler_t *handler);
typedef void (*bus_intr_teardown_t)(device_t *dev, intr_handler_t *handler);

typedef resource_t *(*bus_resource_alloc_t)(device_t *bus, device_t *child,
                                            resource_type_t type, int rid,
                                            rman_addr_t start, rman_addr_t end,
                                            size_t size, unsigned flags);

struct bus_methods {
  bus_intr_setup_t intr_setup;
  bus_intr_teardown_t intr_teardown;
  bus_resource_alloc_t resource_alloc;
};

struct bus_driver {
  driver_t driver;
  bus_methods_t bus;
};

#define BUS_DRIVER(dev) ((bus_driver_t *)((dev)->parent->driver))

static inline void bus_intr_setup(device_t *dev, unsigned num,
                                  intr_handler_t *handler) {
  BUS_DRIVER(dev)->bus.intr_setup(dev, num, handler);
}

static inline void bus_intr_teardown(device_t *dev, intr_handler_t *handler) {
  BUS_DRIVER(dev)->bus.intr_teardown(dev, handler);
}

/*! \brief Allocates resource of size \a size between \a start and \a end.
 *
 * Should be called inside device's \fn attach function.
 *
 * \param dev device which needs resource
 * \param type resource type RT_* defined in rman.h
 * \param rid resource identifier as in \a resource_t structure
 * \param flags RF_* flags defined in rman.h
 */
static inline resource_t *bus_resource_alloc(device_t *dev,
                                             resource_type_t type, int rid,
                                             rman_addr_t start, rman_addr_t end,
                                             size_t size, unsigned flags) {
  return BUS_DRIVER(dev)->bus.resource_alloc(dev->parent, dev, type, rid, start,
                                             end, size, flags);
}

/*! \brief Allocates resource for a device.
 *
 * Basically the same as \sa bus_resource_alloc, but resource placement in
 * memory is chosen by the parent bus.
 */
static inline resource_t *bus_resource_alloc_anywhere(device_t *dev,
                                                      resource_type_t type,
                                                      int rid, size_t size,
                                                      unsigned flags) {
  return BUS_DRIVER(dev)->bus.resource_alloc(dev->parent, dev, type, rid, 0,
                                             RMAN_ADDR_MAX, size, flags);
}

/*! \brief Allocates resource for a device.
 *
 * Basically the same as \sa bus_resource_alloc_anywhere, but resource
 * has to be identifiable by parent bus driver by \param rid.
 */
static inline resource_t *bus_resource_alloc_any(device_t *dev,
                                                 resource_type_t type, int rid,
                                                 unsigned flags) {

  return BUS_DRIVER(dev)->bus.resource_alloc(dev->parent, dev, type, rid, 0,
                                             RMAN_ADDR_MAX, 1, flags);
}

int bus_generic_probe(device_t *bus);

#endif
