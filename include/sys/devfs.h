#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#include <sys/types.h>
#include <sys/refcnt.h>

typedef struct vnode vnode_t;
typedef struct devfs_node devfs_node_t;
typedef struct vnodeops vnodeops_t;
typedef struct devnode devnode_t;
typedef struct file file_t;
typedef struct uio uio_t;

/*
 * Device node is a structure that exposes to user-space an entry point into
 * device driver code through file interface.
 */

/*
 * Called when a user wants to open a file referring to this device node.
 *
 * We have a handful of scenarios to handle here, because some drivers:
 * (1) require that there's exactly one open file (e.g. disks),
 * (2) create a new instance of device node for each opened file (e.g. evdev),
 *     let's call these *device clones*,
 * (3) look up a device node that already exists (e.g. /dev/tty).
 *
 * Arguments:
 *  - `dev`: device node registered by device driver code
 *  - `oflags`: flags that are passed to open(2) and are not maintained
 *              in `fp::f_flags` but still could be useful (e.g. O_TRUNC)
 *  - `fp`: file that needs to be associated with the device node
 *
 * Please note that `fp::f_flags` will tell you if user wants to open this file
 * for read or write (FF_* flags).
 *
 * In case of device cloning (case 2) you should set `fp::f_data` to private
 * instance of device node for the user. Normally `fp::f_data` points to
 * the device node registered by the driver.
 *
 * Returns `ENXIO` if a device could not be configured for use
 * (for instance: when no media is inserted into a drive).
 */
typedef int (*dev_open_t)(devnode_t *dev, file_t *fp, int oflags);

/*
 * Called each time a user closes a file associated with the device node.
 *
 * You can read `fp::f_flags` to learn if the file was opened for read or write.
 */
typedef int (*dev_close_t)(devnode_t *dev, file_t *fp);

/*
 * Read and write routines.
 *
 * If you ensure that there's exactly one opened file that refers to `dev`
 * it doesn't mean there can be at most single thread inside the routines.
 * A user that owns the file is free to fork(2), so both child and parent
 * will have a reference to the file. So even in the simplest use scenario
 * some form of synchronization may be needed here.
 *
 * Please note that `uio::uio_flags` stores `IO_*` flags, so implementation
 * of the routines should be able handle `IO_NONBLOCK`. So if the operation
 * would have blocked, then `EWOULDBLOCK` error code should be returned instead.
 *
 * Please carefully consider if those operations need to put a thread into
 * unbounded interruptible or non-interruptible sleep. A rule of thumb tells
 * that character devices are mostly in the first group, while block devices
 * in the second group.
 *
 * May return error codes from `uiomove` or `cv_wait_intr`.
 */
typedef int (*dev_read_t)(devnode_t *dev, uio_t *uio);
typedef int (*dev_write_t)(devnode_t *dev, uio_t *uio);

/*
 * I/O control reads or modifies device properties.
 *
 * `fflags` is needed to know if a user has opened the file for read or write.
 * If she attempts to modify a property that can only be changed when
 * the device was opened for writing then `EBADF` should be returned.
 */
typedef int (*dev_ioctl_t)(devnode_t *dev, u_long cmd, void *data, int fflags);

typedef enum {
  DT_OTHER = 0,    /* other non-seekable device file */
  DT_SEEKABLE = 1, /* other seekable device file (also a flag) */
  /* TODO: add DT_CONS (!) and DT_DISK. */
} dev_type_t;

/*
 * Please note that the driver is fully responsible for providing
 * synchronization for all methods listed below!
 */
typedef struct devops {
  dev_type_t d_type;   /* device type */
  dev_open_t d_open;   /* prepare device for devfs operations */
  dev_close_t d_close; /* called when file referring to the device is closed */
  dev_read_t d_read;   /* read bytes form a device file */
  dev_write_t d_write; /* write bytes to a device file */
  dev_ioctl_t d_ioctl; /* read or modify device properties */
} devops_t;

typedef struct devnode {
  devops_t *ops;   /* device operation table */
  void *data;      /* device specific data */
  refcnt_t refcnt; /* number of open files referring to this device */

  /* File attributes available through stat(2). Please note that for cloned
   * devices following attributes can only be read with fstat(2).
   *
   * `size` field may be set during first open request, e.g. when we detect
   * media is present, and should be zero'ed out on last close, or when media
   * is removed. */
  mode_t mode; /* node protection mode */
  uid_t uid;   /* file owner */
  gid_t gid;   /* file group */
  size_t size; /* size in bytes (for seekable devices) */
} devnode_t;

/*
 * Device node management for device drivers.
 *
 * TODO(cahir): Following API is too complicated and needlessly exposes
 * `devfs_node_t`. In fact we need something akin to `make_dev_s` and
 * `destroy_dev` (see http://mdoc.su/f/make_dev_s) from FreeBSD, i.e.:
 *
 * - void make_dev_args_init(struct make_dev_args *args);
 * - int make_dev_s(struct make_dev_args *args, struct cdev **cdev,
 *                  const char *fmt, ...);
 * - void destroy_dev(struct	cdev *dev);
 *
 * Directory handling (mkdir / rmdir) should be handled automatically.
 * `devfs_makedev` should accept a full path to device node that is about
 * to be created.
 */

/* If parent is NULL new device will be attached to root devfs directory. */
int devfs_makedev_new(devfs_node_t *parent, const char *name, devops_t *devops,
                      void *data, devnode_t **dev_p);
int devfs_makedir(devfs_node_t *parent, const char *name, devfs_node_t **dir_p);

/* TODO: rewrite all device drivers which use devfs to rely on `devops`
 * instead `vnodeops`. FTTB obsolete device drivers use the following
 * function instead `devfs_makedev_new`. */
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
