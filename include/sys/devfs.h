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
 * The devfs node and the corresponding vnode will no longer be accessible, but
 * there may still be existing references to them. The device driver should
 * provide VOP_RECLAIM() so that it is notified when it is safe to free the
 * devfs node and any driver-private data.
 */
int devfs_unlink(devfs_node_t *dn);

/*
 * Deallocate a devfs node.
 * Only call this function once you are sure that there are no outstanding
 * references to this devfs node, preferably from a device driver's
 * implementation of VOP_RECLAIM().
 */
void devfs_free(devfs_node_t *dn);

/* Get the vnode corresponding to a devfs node.  */
vnode_t *devfs_node_to_vnode(devfs_node_t *dn);

#endif /* !_SYS_DEVFS_H_ */
