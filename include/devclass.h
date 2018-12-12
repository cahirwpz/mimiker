#ifndef _SYS_DEVCLASS_H_
#define _SYS_DEVCLASS_H_

#include <device.h>
#include <queue.h>

typedef struct devclass devclass_t;

struct devclass {
  const char *name;
  driver_t **first_p;
  driver_t **end_p;
};

#define DEVCLASS_CREATE(dc_name)                                               \
  SET_DECLARE(dc_name, driver_t);                                              \
  devclass_t dc_name##_devclass = {.name = (#dc_name),                         \
                                   .first_p = SET_LIMIT(dc_name),              \
                                   .end_p = SET_BEGIN(dc_name)};               \
  SET_ENTRY(devclasses, dc_name##_devclass)

#define DEVCLASS_ADD(dc_name, driver_structure)                                \
  SET_ENTRY(dc_name, driver_structure)

#define DEVCLASS_FOREACH(drv_p, dc)                                            \
  for (drv_p = (dc)->first_p; drv_p < (dc)->end_p; drv_p++)

devclass_t *devclass_find(const char *name);

#endif /* !_SYS_DEVCLASS_H_ */
