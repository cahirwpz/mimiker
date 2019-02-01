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

#define DEV_ATTR_MAX 16 /* maximum number of device attributes */

typedef enum {
  DA_NULL = 0,
  /* FDT attributes */
  DA_FDT_COMPATIBLE,
  DA_FDT_PATH,
  /* resource types */
  DA_IOPORT,
  DA_MEMORY,
  DA_IRQ,
  /* PCI bus identifiers */
  DA_PCI_CLASSCODE,
  DA_PCI_DEVICEID,
  DA_PCI_VENDORID,
} __packed da_tag_t;

typedef short da_id_t;

#define DA_ID_FIRST -1

typedef struct device_attr {
  da_tag_t tag;
  da_id_t id;
  union {
    char *string;
    unsigned number;
    struct {
      intptr_t start, end;
    } range;
  };
} device_attr_t;

struct device {
  /* Device hierarchy. */
  device_t *parent;        /* parent node (bus?) or null (root or pseudo-dev) */
  TAILQ_ENTRY(device) all; /* node on list of all devices */
  TAILQ_ENTRY(device) link; /* node on list of siblings */
  device_list_t children;   /* head of children devices */
  res_list_t resources;     /* head of resources belonging to this device */

  /* Device information and state. */
  device_bus_t bus;
  driver_t *driver;
  // TODO: most likeley we want to get rid of `instance` field
  void *instance; /* used by bus driver to store data in children */
  void *state;    /* memory requested by driver for its state*/
  device_attr_t attributes[DEV_ATTR_MAX]; /* equivalent of FreeBSD's `ivars` */
};

device_t *device_add_child(device_t *dev);
int device_probe(device_t *dev);
int device_attach(device_t *dev);
int device_detach(device_t *dev);

/*! \brief TODO */
device_attr_t *device_get_attr(device_t *dev, da_tag_t tag, da_id_t id);

/* Manually create a device with given driver and parent device. */
device_t *make_device(device_t *parent, driver_t *driver);

/*! \brief Prepares and adds a resource to a device.
 *
 * \note Mostly used in bus drivers. */
void device_add_resource(device_t *dev, resource_t *r, int rid);

/* A universal memory pool to be used by all drivers. */
MALLOC_DECLARE(M_DEV);

#endif /* !_SYS_DEVICE_H_ */
