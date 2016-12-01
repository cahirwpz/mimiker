#include <devfs.h>
#include <cdev.h>
#include <string.h>

cdev_t *devfs_find_device(char *name) {
  cdev_t *cdev;
  TAILQ_FOREACH (cdev, &all_cdevs, cdev_all) {
    if (strncmp(name, cdev->cdev_name, CDEV_NAME_MAX) == 0) {
      return cdev;
    }
  }
  return NULL;
}
