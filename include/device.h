#ifndef _SYS_DEVICE_H_
#define _SYS_DEVICE_H_

#include <queue.h>

typedef struct device device_t;
typedef struct driver driver_t;
typedef TAILQ_HEAD(, device) device_list_t;

typedef void (*d_identify_t)(driver_t *driver, device_t *parent);
typedef int (*d_probe_t)(device_t *dev);
typedef int (*d_attach_t)(device_t *dev);
typedef int (*d_detach_t)(device_t *dev);

struct driver {
  d_identify_t identify; /* add new device to bus */
  d_probe_t probe;       /* probe for specific device(s) */
  d_attach_t attach;     /* attach device to system */
  d_detach_t detach;     /* detach device from system */
  size_t state_size;     /* device->state object size */
};

struct device {
  /* Device hierarchy. */
  device_t *parent;        /* parent node (bus?) or null (root or pseudo-dev) */
  TAILQ_ENTRY(device) all; /* node on list of all devices */
  TAILQ_ENTRY(device) link; /* node on list of siblings */
  device_list_t children;   /* head of children devices */

  /* Device information and state. */
  char *nameunit; /* name+unit e.g. tty0 */
  char *desc;     /* description (set by driver code?) */
  driver_t *driver;
  void *state; /* memory requested by driver */
};

#endif /* _SYS_DEVICE_H_ */
