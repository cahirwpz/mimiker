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

#define ICU_METHODS(dev) ((icu_methods_t *)(dev)->driver->interfaces[DIF_ICU])

static inline void icu_intr_setup(device_t *dev, resource_t *irq,
                                  ih_filter_t *filter, ih_service_t *service,
                                  void *arg, const char *name) {
  while (!ICU_METHODS(dev->parent)) {
    assert(dev->parent->parent);
    dev = dev->parent;
  }
  ICU_METHODS(dev->parent)->intr_setup(dev, irq, filter, service, arg, name);
}

static inline void icu_intr_teardown(device_t *dev, resource_t *irq) {
  while (!ICU_METHODS(dev->parent)) {
    assert(dev->parent->parent);
    dev = dev->parent;
  }
  ICU_METHODS(dev->parent)->intr_teardown(dev, irq);
}

#endif
