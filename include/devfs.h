#ifndef __DEVFS_H__
#define __DEVFS_H__

#include <cdev.h>

/* A very simple devfs stub. Searches the list of all character devices and
 * picks one by name. Returns NULL if no device was found. */
cdev_t *devfs_find_device(char *name);

#endif /* __DEVFS_H__ */
