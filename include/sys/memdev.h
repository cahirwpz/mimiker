#ifndef _SYS_MEMDEV_H_
#define _SYS_MEMDEV_H_

#include <sys/types.h>
#include <sys/device.h>

typedef int(*readblock_t)(device_t *, uint32_t, unsigned char *, uint32_t);
typedef int(*writeblock_t)(device_t *, uint32_t, const uint8_t *, uint32_t);

typedef struct memdev_methods {
  readblock_t read_block;
  writeblock_t write_block;
} memdev_methods_t;

typedef struct memdev_driver {
  driver_t driver;
  memdev_methods_t memdev;
} memdev_driver_t;

#endif