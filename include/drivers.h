#ifndef _SYS_DRIVERS_H_
#define _SYS_DRIVERS_H_

#include <queue.h>

typedef struct device device_t;
typedef struct driver driver_t;

/* The match function should return 1 iff this driver may support the device. */
typedef int bus_match_function(device_t *dev, driver_t *drv);

typedef struct bus_type {
  char *name;
  bus_match_function *match;
  TAILQ_HEAD(, device) devices;
  TAILQ_HEAD(, driver) drivers;
} bus_type_t;

typedef struct device {
  /* Link on list of devices under a bus. */
  TAILQ_ENTRY(device) bus_dev_entry;
  /* Link on list of devices bound to a driver. */
  TAILQ_ENTRY(device) drv_dev_entry;
  /* Link on list of devices beneath our parent. */
  TAILQ_ENTRY(device) children_entry;

  char bus_id[100]; /* The identifier of this device in its parent bus */
  bus_type_t *bus;  /* The bus this device sits on. */
  device_t *parent; /* The parent device. */
  TAILQ_HEAD(, device) children; /* Devices under this one. */
  driver_t *driver;              /* The driver managing this device */
  void *driver_data;             /* Driver-specific. */
} device_t;

/* The probe function must return 1 if the driver can support and wishes to
   claim the device. */
typedef int probe_function(device_t *device);
typedef struct driver {
  TAILQ_ENTRY(driver)
  bus_drv_entry; /* The link on the list of all drivers working with on bus. */
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

void register_builtin_drivers();

#endif /* _SYS_DRIVERS_H_ */
