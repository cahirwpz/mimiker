#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/refcnt.h>
#include <sys/mutex.h>
#include <sys/time.h>

#define DEVFS_NAME_MAX 64

typedef struct vnode vnode_t;
typedef struct devfs_node devfs_node_t;
typedef struct vnodeops vnodeops_t;
typedef struct devsw devsw_t;
typedef struct uio uio_t;
typedef struct devfs_node devfs_node_t;
typedef TAILQ_HEAD(, devfs_node) devfs_node_list_t;

struct devfs_node {
  vnode_t *dn_vnode; /* corresponding v-node */
  char *dn_name;     /* device name */

  /* Node attributes (as in vattr). */
  mode_t dn_mode;      /* node protection mode */
  nlink_t dn_nlinks;   /* number of hard links */
  ino_t dn_ino;        /* node identifier */
  size_t dn_size;      /* device file size (for seekable devices) */
  uid_t dn_uid;        /* file owner */
  gid_t dn_gid;        /* file group */
  timespec_t dn_atime; /* last access time */
  timespec_t dn_mtime; /* last data modification time */
  timespec_t dn_ctime; /* last file status change time */
  mtx_t dn_timelock;   /* guards dn_*time fields */

  union {
    /* Directory-specific data. */
    devfs_node_list_t dn_children; /* list of children nodes */

    /* Device-specific data. */
    struct {
      void *dn_data;     /* device specific data */
      devsw_t *dn_devsw; /* device switch table */
    };
  };

  refcnt_t dn_refcnt;              /* number of open files */
  devfs_node_t *dn_parent;         /* parent devfs node */
  TAILQ_ENTRY(devfs_node) dn_link; /* link on `dn_parent->dn_children` */
};

typedef int (*dev_open_t)(devfs_node_t *dev, int flags);
typedef int (*dev_close_t)(devfs_node_t *dev, int flags);
typedef int (*dev_read_t)(devfs_node_t *dev, uio_t *uio);
typedef int (*dev_write_t)(devfs_node_t *dev, uio_t *uio);
typedef int (*dev_ioctl_t)(devfs_node_t *dev, u_long cmd, void *data,
                           int flags);

/* Put these into `devsw` to provide default implementation for an operation. */
int dev_noopen(devfs_node_t *, int);
int dev_noclose(devfs_node_t *, int);
int dev_noread(devfs_node_t *, uio_t *);
int dev_nowrite(devfs_node_t *, uio_t *);
int dev_noioctl(devfs_node_t *, u_long, void *, int);

typedef enum {
  DT_OTHER = 0,    /* other non-seekable device file */
  DT_SEEKABLE = 1, /* other seekable device file (also a flag) */
  /* TODO: add DT_CONS (!) and DT_DISK. */
} dev_type_t;

/*
 * Character/Block device switch table.
 */
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
                      void *data, devfs_node_t **dn_p);
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
