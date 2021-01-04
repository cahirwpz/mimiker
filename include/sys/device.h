#ifndef _SYS_DEVICE_H_
#define _SYS_DEVICE_H_

#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/linker_set.h>
#include <sys/rman.h>

typedef struct devclass devclass_t;
typedef struct device device_t;
typedef struct driver driver_t;
typedef struct bus_space bus_space_t;
typedef struct intr_handler intr_handler_t;
typedef TAILQ_HEAD(, device) device_list_t;
typedef SLIST_HEAD(, resource_list_entry) resource_list_t;
typedef struct resource_list_entry resource_list_entry_t;

typedef enum { RT_IOPORTS, RT_MEMORY, RT_IRQ } res_type_t;

struct resource_list_entry {
  SLIST_ENTRY(resource_list_entry) link;
  resource_t *res; /* the actual resource */
  res_type_t type; /* resource type */
  /* auxiliary data associated with a resource */
  union {
    intr_handler_t *handler;
  };
  int rid; /* resource identifier */
};

/* Driver that returns the highest value from its probe action
 * will be selected for attach action. */
typedef int (*d_probe_t)(device_t *dev);
/* Returns value from <errno.h> on failure, 0 on success. */
typedef int (*d_attach_t)(device_t *dev);
typedef int (*d_detach_t)(device_t *dev);

/* Update this section if you add any new driver interface */
typedef enum {
  DIF_BUS,
  DIF_PCI_BUS,
  DIF_COUNT /* this must be the last item */
} drv_if_t;

struct driver {
  const char *desc;            /* short driver description */
  size_t size;                 /* device->state object size */
  d_probe_t probe;             /* probe for specific device(s) */
  d_attach_t attach;           /* attach device to system */
  d_detach_t detach;           /* detach device from system */
  void *interfaces[DIF_COUNT]; /* pointers to device methods structures */
};

typedef enum { DEV_BUS_NONE, DEV_BUS_PCI, DEV_BUS_ISA } device_bus_t;

struct device {
  /* Device hierarchy. */
  device_t *parent; /* parent node (bus?) or null (root or pseudo-dev) */
  TAILQ_ENTRY(device) link; /* node on list of siblings */
  device_list_t children;   /* head of children devices */

  /* Device information and state. */
  device_bus_t bus; /* (for children) type of bus we're attached to */
  driver_t *driver;
  devclass_t *devclass; /* (for buses) device class of children */
  int unit;
  void *instance;            /* used by bus driver to store data in children */
  void *state;               /* memory requested by driver for its state */
  resource_list_t resources; /* used by driver, assigned by parent bus */
};

/*! \brief Called during kernel initialization. */
void init_devices(void);

device_t *device_alloc(int unit);
device_t *device_add_child(device_t *parent, int unit);
int device_probe(device_t *dev);
int device_attach(device_t *dev);
int device_detach(device_t *dev);

/*! \brief Add a resource entry to resource list. */
void device_add_resource(device_t *dev, res_type_t type, int rid,
                         rman_addr_t start, rman_addr_t end, size_t size,
                         res_flags_t flags);

#define device_add_memory(dev, rid, start, size)                               \
  device_add_resource((dev), RT_MEMORY, (rid), (start), (start) + (size)-1,    \
                      (size), 0)

#define device_add_ioports(dev, rid, start, size)                              \
  device_add_resource((dev), RT_IOPORTS, (rid), (start), (start) + (size)-1,   \
                      (size), 0)

#define device_add_irq(dev, rid, irq)                                          \
  device_add_resource((dev), RT_IRQ, (rid), (irq), (irq), 1, RF_SHAREABLE)

/*! \brief Take a resource which is assigned to device by parent bus. */
resource_t *device_take_resource(device_t *dev, res_type_t type, int rid,
                                 res_flags_t flags);

#define device_take_memory(dev, rid, flags)                                    \
  device_take_resource((dev), RT_MEMORY, (rid), (flags))

#define device_take_ioports(dev, rid, flags)                                   \
  device_take_resource((dev), RT_IOPORTS, (rid), (flags))

#define device_take_irq(dev, rid, flags)                                       \
  device_take_resource((dev), RT_IRQ, (rid), (flags))

/*! \brief Return resource list entry of device `dev` correspoinding
 * to resource `res`. */
resource_list_entry_t *resource_rle(resource_t *res, device_t *dev);

/*! \brief Return resource identifier associated with resource `res`
 * in the context of device `dev`. */
static inline int resource_get_rid(resource_t *res, device_t *dev) {
  return resource_rle(res, dev)->rid;
}

/*! \brief Return interrupt handler associated with resource `res`
 * in the context of device `dev`. */
static inline intr_handler_t *resource_get_handler(resource_t *res,
                                                   device_t *dev) {
  return resource_rle(res, dev)->handler;
}

/*! \brief Set interrupt handler associated with resource `res`
 * in the context of device `dev`. */
static inline void resource_set_handler(resource_t *res, device_t *dev,
                                        intr_handler_t *handler) {
  resource_rle(res, dev)->handler = handler;
}

/* A universal memory pool to be used by all drivers. */
KMALLOC_DECLARE(M_DEV);

#endif /* !_SYS_DEVICE_H_ */
