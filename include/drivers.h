#ifndef _SYS_DRIVERS_H_
#define _SYS_DRIVERS_H_

#include <queue.h>

typedef struct device device_t;
typedef struct driver driver_t;

typedef int bus_match(device_t *dev, driver_t *drv);

typedef struct bus_type {
  char *name;
  bus_match *match;
  TAILQ_HEAD(, device) devices;
  TAILQ_HEAD(, device) drivers;
} bus_type_t;

typedef struct device {
  char bus_id[100];  /* The identifier of this device in its parent bus */
  bus_type_t *bus;   /* The bus this device sits on. */
  driver_t *driver;  /* The driver managing this device */
  void *driver_data; /* Driver-specific. */
} device_t;

typedef int probe_function(device_t *device);
typedef struct driver {
  char *name;
  bus_type_t *bus; /* The type of bus this driver works with */
  TAILQ_HEAD(, device)
  devices; /* list of devices currently bound by this driver */
  probe_function *probe;
} driver_t;

/* Implementation for these two is provided by the driver core. Useful for
 * writing buses. */
int bus_for_each_dev(bus_type_t *bus, void *data,
                     int (*fn)(device_t *, void *));
int bus_for_each_drv(bus_type_t *bus, void *data,
                     int (*fn)(device_t *, void *));

int bus_register(bus_type_t *bus);
int device_register(device_t *device);
int driver_register(driver_t *driver);

#endif /* _SYS_DRIVERS_H_ */
