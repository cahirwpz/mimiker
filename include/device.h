#ifndef _SYS_DEVICE_H_
#define _SYS_DEVICE_H_

#include <queue.h>
#include <malloc.h>
#include <linker_set.h>
#include <rman.h>

typedef struct device device_t;
typedef struct driver driver_t;
typedef struct resource resource_t;
typedef struct bus_space bus_space_t;
typedef TAILQ_HEAD(, device) device_list_t;

typedef void (*d_identify_t)(driver_t *driver, device_t *parent);
typedef int (*d_probe_t)(device_t *dev);
typedef int (*d_attach_t)(device_t *dev);
typedef int (*d_detach_t)(device_t *dev);

#define MAX_DEV_NAME_LEN 32
#define PATHBUF_LEN 100

struct driver {
  const char *desc;      /* short driver description */
  size_t size;           /* device->state object size */
  d_identify_t identify; /* add new device to bus */
  d_probe_t probe;       /* probe for specific device(s) */
  d_attach_t attach;     /* attach device to system */
  d_detach_t detach;     /* detach device from system */
};

#define DRIVER_ADD(name) SET_ENTRY(driver_table, name)

typedef enum { DEV_BUS_NONE, DEV_BUS_PCI, DEV_BUS_ISA } device_bus_t;

struct device {
  /* Device hierarchy. */
  device_t *parent;        /* parent node (bus?) or null (root or pseudo-dev) */
  TAILQ_ENTRY(device) all; /* node on list of all devices */
  TAILQ_ENTRY(device) link; /* node on list of siblings */
  device_list_t children;   /* head of children devices */
  res_list_t resources;     /* head of resources belonging to this device */

  /* Device information and state. */
  int unit;
  char *name;
  device_bus_t bus;
  driver_t *driver;
  void *instance; /* used by bus driver to store data in children */
  void *state;    /* memory requested by driver for its state*/
};

device_t *device_add_child(device_t *dev);
int device_probe(device_t *dev);
int device_attach(device_t *dev);
int device_detach(device_t *dev);

/* Create a device with given \parent device, \driver
 * and set device's \name and \unit  */
device_t *make_device(device_t *parent, driver_t *driver, char *name, int unit);

/*! \brief Prepares and adds a resource to a device.
 *
 * \note Mostly used in bus drivers. */
void device_add_resource(device_t *dev, resource_t *r, int rid);

/* Construct device's full name and store it in \buff */
int device_get_fullname(device_t *dev, char *buff, int size);

/* Construct device's full path to the root store it in \buff */
void device_construct_fullpath(device_t *dev, char *buff, size_t size);

/* A universal memory pool to be used by all drivers. */
MALLOC_DECLARE(M_DEV);

#endif /* !_SYS_DEVICE_H_ */
