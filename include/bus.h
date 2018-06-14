#ifndef _SYS_BUS_H_
#define _SYS_BUS_H_

#include <rman.h>
#include <device.h>

typedef struct bus_space bus_space_t;
typedef struct bus_methods bus_methods_t;
typedef struct bus_driver bus_driver_t;
typedef struct intr_handler intr_handler_t;

/* `bus space` accessor routines */
typedef uint8_t (*bus_space_read_1_t)(resource_t *handle, unsigned offset);
typedef void (*bus_space_write_1_t)(resource_t *handle, unsigned offset,
                                    uint8_t value);
typedef uint16_t (*bus_space_read_2_t)(resource_t *handle, unsigned offset);
typedef void (*bus_space_write_2_t)(resource_t *handle, unsigned offset,
                                    uint16_t value);
typedef uint32_t (*bus_space_read_4_t)(resource_t *handle, unsigned offset);
typedef void (*bus_space_write_4_t)(resource_t *handle, unsigned offset,
                                    uint32_t value);
typedef void (*bus_space_read_region_1_t)(resource_t *handle, unsigned offset,
                                          uint8_t *dst, size_t count);
typedef void (*bus_space_write_region_1_t)(resource_t *handle, unsigned offset,
                                           const uint8_t *src, size_t count);

/* `bus space` describes a method to access hardware resources mapped at some
 * address. We make no distinction between different kinds of physical address
 * spaces. Same hardware resource can be accessed in many different ways
 * depending on which bus it was attached to (e.g. I/O ports vs. MMIO) */
struct bus_space {
  bus_space_read_1_t read_1;   /* how to read one byte? */
  bus_space_write_1_t write_1; /* how to write one byte? */
  bus_space_read_2_t read_2;   /* how to read word? */
  bus_space_write_2_t write_2; /* how to write word? */
  bus_space_read_4_t read_4;   /* how to read double word? */
  bus_space_write_4_t write_4; /* how to write double word? */
  bus_space_read_region_1_t read_region_1;
  bus_space_write_region_1_t write_region_1;
};

#define BUS_SPACE_DECLARE(name) extern bus_space_t name[1]

#define RESOURCE_DECLARE(name) extern resource_t name[1]

static inline uint8_t bus_space_read_1(resource_t *handle, unsigned offset) {
  return handle->r_bus_space->read_1(handle, offset);
}

static inline void bus_space_write_1(resource_t *handle, unsigned offset,
                                     uint8_t value) {
  handle->r_bus_space->write_1(handle, offset, value);
}

static inline uint16_t bus_space_read_2(resource_t *handle, unsigned offset) {
  return handle->r_bus_space->read_2(handle, offset);
}

static inline void bus_space_write_2(resource_t *handle, unsigned offset,
                                     uint16_t value) {
  handle->r_bus_space->write_2(handle, offset, value);
}

static inline uint32_t bus_space_read_4(resource_t *handle, unsigned offset) {
  return handle->r_bus_space->read_4(handle, offset);
}

static inline void bus_space_write_4(resource_t *handle, unsigned offset,
                                     uint32_t value) {
  handle->r_bus_space->write_4(handle, offset, value);
}

static inline void bus_space_read_region_1(resource_t *handle, unsigned offset,
                                           uint8_t *dst, size_t count) {
  return handle->r_bus_space->read_region_1(handle, offset, dst, count);
}

static inline void bus_space_write_region_1(resource_t *handle, unsigned offset,
                                            const uint8_t *src, size_t count) {
  handle->r_bus_space->write_region_1(handle, offset, src, count);
}

typedef void (*bus_intr_setup_t)(device_t *dev, unsigned num,
                                 intr_handler_t *handler);
typedef void (*bus_intr_teardown_t)(device_t *dev, intr_handler_t *handler);

typedef resource_t *(*bus_resource_alloc_t)(device_t *bus, device_t *child,
                                            int type, int rid,
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
static inline resource_t *bus_resource_alloc(device_t *dev, int type, int rid,
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
static inline resource_t *bus_resource_alloc_anywhere(device_t *dev, int type,
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
static inline resource_t *bus_resource_alloc_any(device_t *dev, int type,
                                                 int rid, unsigned flags) {

  return BUS_DRIVER(dev)->bus.resource_alloc(dev->parent, dev, type, rid, 0,
                                             RMAN_ADDR_MAX, 1, flags);
}

static inline void bus_generic_new_resource_init(resource_t *r, device_t *dev,
                                                 int type, int rid,
                                                 bus_space_t *bus_space) {
  r->r_owner = dev;
  r->r_id = rid;
  r->r_type = type;
  r->r_bus_space = bus_space;
  LIST_INSERT_HEAD(&dev->resources, r, r_device);
}

int bus_generic_probe(device_t *bus);

#endif
