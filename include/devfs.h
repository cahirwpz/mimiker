#ifndef __SYS_DEVFS_H__
#define __SYS_DEVFS_H__

#include <mount.h>
#include <vnode.h>

/* Registers the devfs file system in the vfs. */
void devfs_init();

#define DEVFS_DEVICE_NAME_MAX 64

/* Installs a new device into the devfs */
int devfs_install(const char *name, vnode_t *device);

#endif /* __SYS_DEVFS_H__ */
