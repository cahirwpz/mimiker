#ifndef _SYS_ISA_H_
#define _SYS_ISA_H_

#include <sys/device.h>
#include <sys/resource.h>
#include <sys/rman.h>
#include <sys/interrupt.h>

#define ICU_LEN 16 /* number of ISA IRQs */

extern bus_driver_t intel_isa_bus;

#endif