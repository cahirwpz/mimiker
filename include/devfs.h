#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#define DEVFS_NAME_MAX 64

typedef struct vnode vnode_t;

/* Registers the devfs file system in the vfs. */
void devfs_init();

/* Installs a new device into the devfs */
int devfs_install(const char *name, vnode_t *device);

#endif /* !_SYS_DEVFS_H_ */
