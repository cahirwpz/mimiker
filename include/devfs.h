#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#define DEVFS_NAME_MAX 64

typedef struct vnode vnode_t;

#define DEVFS_INSTALL_FLAG_NUMBERED 0x1

/* Installs a new device into the devfs */
int devfs_install(const char *name, vnode_t *device, unsigned flags);

#endif /* !_SYS_DEVFS_H_ */
