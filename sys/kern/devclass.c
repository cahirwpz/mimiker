#include <sys/device.h>
#include <sys/devclass.h>
#include <sys/libkern.h>

SET_DECLARE(devclasses, devclass_t);

devclass_t *devclass_find(const char *name) {
  devclass_t **dc_p;

  SET_FOREACH (dc_p, devclasses) {
    if (strcmp((*dc_p)->name, name) == 0)
      return *dc_p;
  }
  return NULL;
}
