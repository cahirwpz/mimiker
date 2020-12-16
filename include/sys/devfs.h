#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#define DEVFS_NAME_MAX 64

typedef struct vnode vnode_t;
typedef struct devfs_node devfs_node_t;
typedef struct vnodeops vnodeops_t;

/* If parent is NULL new device will be attached to root devfs directory.
 * If vnode_p is not NULL, on success *vnode_p will point to the vnode of
 * the newly created device. */
int devfs_makedev(devfs_node_t *parent, const char *name, vnodeops_t *vops,
                  void *data, vnode_t **vnode_p);
int devfs_makedir(devfs_node_t *parent, const char *name, devfs_node_t **dir_p);
void *devfs_node_data(vnode_t *vnode);

#endif /* !_SYS_DEVFS_H_ */
