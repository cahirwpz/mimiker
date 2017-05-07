#ifndef _SYS_DEVICE_H_
#define _SYS_DEVICE_H_

#include <queue.h>
#include <malloc.h>
#include <linker_set.h>

typedef struct device device_t;
typedef struct driver driver_t;
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

struct device {
  /* Device hierarchy. */
  device_t *parent;        /* parent node (bus?) or null (root or pseudo-dev) */
  TAILQ_ENTRY(device) all; /* node on list of all devices */
  TAILQ_ENTRY(device) link; /* node on list of siblings */
  device_list_t children;   /* head of children devices */

  /* Device information and state. */
  driver_t *driver;
  void *instance; /* used by bus driver to store data in children */
  void *state;    /* memory requested by driver for its state*/
};

void driver_init();

device_t *device_add_child(device_t *dev);
int device_probe(device_t *dev);
int device_attach(device_t *dev);
int device_detach(device_t *dev);

/* A universal memory pool to be used by all drivers. */
MALLOC_DECLARE(M_DEV);

#endif /* _SYS_DEVICE_H_ */
