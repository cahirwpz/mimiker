#ifndef _SYS_ISA_H_
#define _SYS_ISA_H_

#include <bus.h>
#include <device.h>

#define ISA_BUS_IOPORTS_SIZE 0x10000

typedef struct isa_device {
  /* A reference to this device's isa bus. */
  resource_t *isa_bus;
} isa_device_t;

static inline isa_device_t *isa_device_of(device_t *device) {
  return (device->bus == DEV_BUS_ISA) ? device->instance : NULL;
}

#endif /* _SYS_ISA_H_ */
