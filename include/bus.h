#ifndef _SYS_BUS_H_
#define _SYS_BUS_H_

#include <common.h>

typedef struct resource resource_t;
typedef struct bus_space bus_space_t;

/* `bus space` accessor routines */
typedef uint8_t (*bus_space_read_1_t)(resource_t *handle, unsigned offset);
typedef void (*bus_space_write_1_t)(resource_t *handle, unsigned offset,
                                    uint8_t value);

/* `bus space` describes a method to access hardware resources mapped at some
 * address. We make no distinction between different kinds of physical address
 * spaces. Same hardware resource can be accessed in many different ways
 * depending on which bus it was attached to (e.g. I/O ports vs. MMIO) */
struct bus_space {
  bus_space_read_1_t read_1;   /* how to read one byte? */
  bus_space_write_1_t write_1; /* how to write one byte? */
};

#define BUS_SPACE_DEFINE(name)                                                 \
  bus_space_t name##_bus_space[1] = {{                                         \
    .read_1 = name##_read_1, .write_1 = name##_write_1,                        \
  }}

#define BUS_SPACE_DECLARE(name) extern bus_space_t name##_bus_space[1]

/* `resource` describes a range of addresses where a resource is mapped within
 * given bus space. A driver will use addresses from `r_start` to `r_end` and
 * `r_bus_space` routines to access hardware resource, so that the actual
 * driver code is not tied to way handled device is attached to the system. */
struct resource {
  bus_space_t *r_bus_space; /* bus space accessor descriptor */
  intptr_t r_addr;  /* address of a set of resources mapped onto the bus */
  intptr_t r_start; /* first address of the resource */
  intptr_t r_end;   /* last address of the resource */
};

#define RESOURCE_DECLARE(name) extern resource_t name[1]

static inline uint8_t bus_space_read_1(resource_t *handle, unsigned offset) {
  return handle->r_bus_space->read_1(handle, offset);
}

static inline void bus_space_write_1(resource_t *handle, unsigned offset,
                                     uint8_t value) {
  handle->r_bus_space->write_1(handle, offset, value);
}

#endif
