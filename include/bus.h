#ifndef _SYS_BUS_H_
#define _SYS_BUS_H_

#include <common.h>
#include <device.h>

typedef struct resource resource_t;
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

/* `resource` describes a range of addresses where a resource is mapped within
 * given bus space. A driver will use addresses from `r_start` to `r_end` and
 * `r_bus_space` routines to access hardware resource, so that the actual
 * driver code is not tied to way handled device is attached to the system. */

#define RT_UNKNOWN 0
#define RT_MEMORY 1
#define RT_IOPORTS 2

#define RF_NONE 0
/* According to PCI specification prefetchable bit is CLEAR when memory mapped
 * resource contains locations with read side-effects or locations in which the
 * device does not tolerate write merging. */
#define RF_PREFETCHABLE 1

struct resource {
  bus_space_t *r_bus_space; /* bus space accessor descriptor */
  void *r_owner;            /* pointer to device that owns this resource */
  intptr_t r_start;         /* first physical address of the resource */
  intptr_t r_end; /* last (inclusive) physical address of the resource */
  unsigned r_type;
  unsigned r_flags;
  int r_id; /* (optional) resource identifier */
};

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

struct bus_methods {
  bus_intr_setup_t intr_setup;
  bus_intr_teardown_t intr_teardown;
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

int bus_generic_probe(device_t *bus);

#endif
