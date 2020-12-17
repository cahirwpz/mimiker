#ifndef _ICU_METHODS_H_
#define _ICU_METHODS_H_

#include <sys/device.h>
#include <sys/resource.h>
#include <sys/interrupt.h>

typedef struct icu_methods {
  void (*intr_setup)(device_t *dev, resource_t *irq, ih_filter_t *filter,
                     ih_service_t *service, void *arg, const char *name);
  void (*intr_teardown)(device_t *dev, resource_t *irq);
} icu_methods_t;

#define ICU_METHODS(dev) (*(icu_methods_t *)(dev)->driver->interfaces[DIF_ICU])

static inline void icu_intr_setup(device_t *dev, resource_t *irq,
                                  ih_filter_t *filter, ih_service_t *service,
                                  void *arg, const char *name) {
  assert (dev->parent);
  if (!dev->parent->driver->interfaces[DIF_ICU]) {
    icu_intr_setup(dev, irq, filter, service, arg, name);
    return;
  }
  ICU_METHODS(dev->parent).intr_setup(dev, irq, filter, service, arg, name);
}

static inline void icu_intr_teardown(device_t *dev, resource_t *irq) {
  assert (dev->parent);
  if (!dev->parent->driver->interfaces[DIF_ICU]) {
    icu_intr_teardown(dev, irq);
    return;
  }
  ICU_METHODS(dev->parent).intr_teardown(dev, irq);
}

#endif