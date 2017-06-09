#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#include <vnode.h>

#define DEVFS_NAME_MAX 64

/* Installs a new device into the devfs */
int devfs_install(const char *name, vnodetype_t type, vnodeops_t *vops,
                  void *data);

#endif /* !_SYS_DEVFS_H_ */
