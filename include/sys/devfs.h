#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#define DEVFS_NAME_MAX 64

typedef struct vnode vnode_t;
typedef struct devfs_node devfs_node_t;
typedef struct vnodeops vnodeops_t;

/* If parent is NULL new device will be attached to root devfs directory.
 * If dnode_p is not NULL, on success *dnode_p will point to the devfs_node_t of
 * the newly created device. */
int devfs_makedev(devfs_node_t *parent, const char *name, vnodeops_t *vops,
                  void *data, devfs_node_t **dnode_p);
int devfs_makedir(devfs_node_t *parent, const char *name, devfs_node_t **dir_p);

/*
 * Remove a node from the devfs tree.
 * The corresponding vnode will no longer be accessible, but there may still be
 * existing references to it. The device driver should provide vop_reclaim so
 * that it is notified when it is safe to delete the any driver-private data.
 */
int devfs_remove(devfs_node_t *dn);

/* Get the vnode corresponding to a devfs node.  */
vnode_t *devfs_node_to_vnode(devfs_node_t *dn);

#endif /* !_SYS_DEVFS_H_ */
