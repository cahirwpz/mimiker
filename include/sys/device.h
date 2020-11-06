#ifndef _SYS_DEVICE_H_
#define _SYS_DEVICE_H_

#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/linker_set.h>
#include <sys/rman.h>

typedef struct devclass devclass_t;
typedef struct device device_t;
typedef struct driver driver_t;
typedef struct resource resource_t;
typedef struct bus_space bus_space_t;
typedef TAILQ_HEAD(, device) device_list_t;

typedef int (*d_probe_t)(device_t *dev);
typedef int (*d_attach_t)(device_t *dev);
typedef int (*d_detach_t)(device_t *dev);

struct driver {
  const char *desc;  /* short driver description */
  size_t size;       /* device->state object size */
  d_probe_t probe;   /* probe for specific device(s) */
  d_attach_t attach; /* attach device to system */
  d_detach_t detach; /* detach device from system */
};

typedef enum { DEV_BUS_NONE, DEV_BUS_PCI, DEV_BUS_ISA } device_bus_t;

struct device {
  /* Device hierarchy. */
  device_t *parent; /* parent node (bus?) or null (root or pseudo-dev) */
  TAILQ_ENTRY(device) link; /* node on list of siblings */
  device_list_t children;   /* head of children devices */

  /* Device information and state. */
  device_bus_t bus;
  driver_t *driver;
  devclass_t *devclass;
  int unit;
  void *instance; /* used by bus driver to store data in children */
  void *state;    /* memory requested by driver for its state*/
};

/*! \brief Called during kernel initialization. */
void init_devices(void);

device_t *device_add_child(device_t *parent, devclass_t *dc, int unit);
int device_probe(device_t *dev);
int device_attach(device_t *dev);
int device_detach(device_t *dev);

/* A universal memory pool to be used by all drivers. */
KMALLOC_DECLARE(M_DEV);

#endif /* !_SYS_DEVICE_H_ */
