#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#include <sys/types.h>

typedef struct vnode vnode_t;
typedef struct devfs_node devfs_node_t;
typedef struct vnodeops vnodeops_t;
typedef struct devsw devsw_t;
typedef struct uio uio_t;

/*
 * Device node.
 *
 * XXX: should it be read-only for device drivers code ?
 */

#define DEVFS_NAME_MAX 64

typedef struct dev_file {
  char name[DEVFS_NAME_MAX]; /* device file name */
  devsw_t *devsw;            /* device switch table */
  void *data;                /* device specific data */
  mode_t mode;               /* node protection mode */
  size_t size;               /* device file size (for seekable devices) */
  uid_t uid;                 /* file owner */
  gid_t gid;                 /* file group */
} dev_file_t;

/*
 * Device switch table and its functions.
 */

typedef int (*dev_open_t)(dev_file_t *dev, int flags);
typedef int (*dev_close_t)(dev_file_t *dev, int flags);
typedef int (*dev_read_t)(dev_file_t *dev, uio_t *uio);
typedef int (*dev_write_t)(dev_file_t *dev, uio_t *uio);
typedef int (*dev_ioctl_t)(dev_file_t *dev, u_long cmd, void *data, int flags);

/* Put these into `devsw` to provide default implementation for an operation. */
int dev_noopen(dev_file_t *, int);
int dev_noclose(dev_file_t *, int);
int dev_noread(dev_file_t *, uio_t *);
int dev_nowrite(dev_file_t *, uio_t *);
int dev_noioctl(dev_file_t *, u_long, void *, int);

typedef enum {
  DT_OTHER = 0,    /* other non-seekable device file */
  DT_SEEKABLE = 1, /* other seekable device file (also a flag) */
  /* TODO: add DT_CONS (!) and DT_DISK. */
} dev_type_t;

struct devsw {
  dev_type_t d_type;   /* device type */
  dev_open_t d_open;   /* prepare device for devfs operations */
  dev_close_t d_close; /* called when file referring to the device is closed */
  dev_read_t d_read;   /* read bytes form a device file */
  dev_write_t d_write; /* write bytes to a device file */
  dev_ioctl_t d_ioctl; /* read or modify device properties */
};

/* If parent is NULL new device will be attached to root devfs directory. */
int devfs_makedev_new(devfs_node_t *parent, const char *name, devsw_t *devsw,
                      void *data, dev_file_t **dev_p);
int devfs_makedir(devfs_node_t *parent, const char *name, devfs_node_t **dir_p);

/* TODO: rewrite all device drivers which use devfs to rely on devsw
 * instead vnodeops. FTTB, obsolete device drivers use the following
 * function instead `devfs_makedev`. */
int devfs_makedev(devfs_node_t *parent, const char *name, vnodeops_t *vops,
                  void *data, vnode_t **vnode_p);

/* TODO: remove it after rewriting drivers. */
void *devfs_node_data(vnode_t *vnode);

/*
 * Remove a node from the devfs tree.
 *
 * The devfs node and the corresponding vnode will no longer be accessible,
 * but there may still be existing references to them.
 *
 * This function should be called form the driver's detach function.
 */
int devfs_unlink(devfs_node_t *dn);

/* TODO: remove it after rewriting drivers. */
void devfs_free(devfs_node_t *dn);

#endif /* !_SYS_DEVFS_H_ */
