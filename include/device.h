#ifndef _SYS_DEVICE_H_
#define _SYS_DEVICE_H_

#include <queue.h>
#include <malloc.h>
#include <linker_set.h>
#include <rman.h>

typedef struct device device_t;
typedef struct devprops devprops_t;
typedef struct devprop_attr devprop_attr_t;
typedef struct devprop_res devprop_res_t;
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

// TODO: There should be more key types
typedef enum {
  CLASSCODE,
  COMPATIBLE,
  DEVICEID,
  FDT_PATH,
  VENDORID,
} devprop_attr_type_t;

// TODO: There should be more key types
typedef enum {
  PCIBAR,
  IOPORT, // TODO: How to handle IOPORT1 and IOPORT2 ?
  IRQPIN,
  IRQLINE,
} devprop_res_type_t;

typedef union {
  char *str_value;
  uint8_t uint8_value;
  uint16_t uint16_value; // TODO: naming?
} devprop_attr_val_t;

typedef union {
  char *str_value;
  uint8_t uint8_value;
  uint16_t uint16_value;
  void *bar;
  // pci_bar_t *bar; // TODO: do we want PCI BAR ? circular import right now
} devprop_res_val_t;

typedef struct {
  uint32_t rid;
  devpropt_res_val_t val;
} devprop_res_val_rid_t;

struct devprop_attr {
  devprop_attr_key_t key;
  devprop_attr_val_t *value;
};

struct devprop_res {
  devprop_res_key_t key;
  devprop_res_val_rid_t *value;
};

struct devprops {
  devprop_attr_t *attrs;
  devprop_res_t *resources;
};

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
  void *instance;    /* used by bus driver to store data in children */
  void *state;       /* memory requested by driver for its state*/
  devprops_t *props; // equivalent of FreeBSD's `ivars`
};

device_t *device_add_child(device_t *dev);
int device_probe(device_t *dev);
int device_attach(device_t *dev);
int device_detach(device_t *dev);

devprop_attr_t *get_device_prop_attr(device_t *dev, devprop_attr_key_t key);
devprop_res_t *get_device_prop_res(device_t *dev, devprop_res_key_t key);

void set_device_prop_attr(device_t *dev, devprop_attr_key_t key,
                          devprop_attr_val_t *value);
void set_device_prop_res(device_t *dev, devprop_res_key_t key,
                         devprop_res_val_t *value);

/* Manually create a device with given driver and parent device. */
device_t *make_device(device_t *parent, driver_t *driver);

/*! \brief Prepares and adds a resource to a device.
 *
 * \note Mostly used in bus drivers. */
void device_add_resource(device_t *dev, resource_t *r, int rid);

/* A universal memory pool to be used by all drivers. */
MALLOC_DECLARE(M_DEV);

#endif /* !_SYS_DEVICE_H_ */
