#ifndef _SYS_DEVCLASS_H_
#define _SYS_DEVCLASS_H_

#include <sys/device.h>

typedef struct devclass devclass_t;

struct devclass {
  const char *name;
  driver_t **first_p;
  driver_t **end_p;
};

#define DEVCLASS(dc_name) dc_name##_devclass

/* Create a new devclass with name \dc_name which will store a linker set
 * of drivers associated with the new devclass. */
#define DEVCLASS_CREATE(dc_name)                                               \
  SET_DECLARE(dc_name, driver_t);                                              \
  devclass_t dc_name##_devclass = {.name = (#dc_name),                         \
                                   .first_p = SET_BEGIN(dc_name),              \
                                   .end_p = SET_LIMIT(dc_name)};               \
  SET_ENTRY(devclasses, dc_name##_devclass)

#define DEVCLASS_DECLARE(dc_name) extern devclass_t dc_name##_devclass

/* Add new driver to the devclass named \dc_name */
#define DEVCLASS_ENTRY(dc_name, driver) SET_ENTRY(dc_name, driver)

/* Iterate over devclass's drivers
 *
 * drv_p: driver_t**
 * dc: devclass_t*
 */
#define DEVCLASS_FOREACH(drv_p, dc)                                            \
  for (drv_p = (dc)->first_p; drv_p < (dc)->end_p; drv_p++)

/* Find devclass by name */
devclass_t *devclass_find(const char *name);

#endif /* !_SYS_DEVCLASS_H_ */
