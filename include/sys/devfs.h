#ifndef _SYS_DEVFS_H_
#define _SYS_DEVFS_H_

#include <sys/types.h>
#include <sys/queue.h>

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
  mode_t dn_mode;    /* node protection mode */
  nlink_t dn_nlinks; /* number of hard links */
  ino_t dn_ino;      /* node identifier */
  size_t dn_size;    /* device file size (for seekable devices) */
  uid_t dn_uid;      /* file owner */
  gid_t dn_gid;      /* file group */

  union {
    /* Directory-specific data. */
    devfs_node_list_t dn_children; /* list of children nodes */

    /* Device-specific data. */
    struct {
      void *dn_data;     /* device specific data */
      devsw_t *dn_devsw; /* device switch table */
    };
  };

  devfs_node_t *dn_parent;         /* parent devfs node */
  TAILQ_ENTRY(devfs_node) dn_link; /* link on `dn_parent->dn_children` */
};

typedef int (*dev_open_t)(devfs_node_t *dev, int flags);
typedef int (*dev_close_t)(devfs_node_t *dev, int flags);
typedef int (*dev_reclaim_t)(devfs_node_t *dev);
typedef int (*dev_read_t)(devfs_node_t *dev, uio_t *uio);
typedef int (*dev_write_t)(devfs_node_t *dev, uio_t *uio);
/* TODO: add `dev_strategy_t`. */
typedef int (*dev_ioctl_t)(devfs_node_t *dev, u_long cmd, void *data,
                           int flags);

typedef enum {
  DT_OTHER = 0,    /* other non-seekable device file */
  DT_SEEKABLE = 1, /* other seekable device file (also a flag) */
  /* TODO: add DT_CONS (!) and DT_DISK. */
} dev_type_t;

/*
 * Character/Block device switch table.
 */
struct devsw {
  dev_open_t d_open;   /* prepare device for devfs operations */
  dev_close_t d_close; /* called when file referring to the device is closed */
  dev_reclaim_t d_reclaim; /* free the devfs node and driver-private data */
  union {
    /* Character device I/O. */
    struct {
      dev_read_t d_read;   /* read bytes form a device file */
      dev_write_t d_write; /* write bytes to a device file */
    };

    /* Block device I/O. */
    /* TODO: d_startegy. */
  };
  dev_ioctl_t d_ioctl; /* read or modify device properties */
  dev_type_t d_type;
};

static inline int DOP_OPEN(devfs_node_t *dn, int flags) {
  return dn->dn_devsw->d_open(dn, flags);
}

static inline int DOP_CLOSE(devfs_node_t *dn, int flags) {
  return dn->dn_devsw->d_close(dn, flags);
}

static inline int DOP_RECLAIM(devfs_node_t *dn) {
  return dn->dn_devsw->d_reclaim(dn);
}

static inline int DOP_READ(devfs_node_t *dn, uio_t *uio) {
  return dn->dn_devsw->d_read(dn, uio);
}

static inline int DOP_WRITE(devfs_node_t *dn, uio_t *uio) {
  return dn->dn_devsw->d_write(dn, uio);
}

static inline int DOP_IOCTL(devfs_node_t *dn, u_long cmd, void *data,
                            int flags) {
  return dn->dn_devsw->d_ioctl(dn, cmd, data, flags);
}

/* If parent is NULL new device will be attached to root devfs directory. */
int devfs_makedev(devfs_node_t *parent, const char *name, devsw_t *devsw,
                  void *data, vnode_t **vnode_p);
int devfs_makedir(devfs_node_t *parent, const char *name, devfs_node_t **dir_p);

/* TODO: rewrite all device drivers which use devfs to rely on devsw
 * instead vnodeops. FTTB, obsolete device drivers use the following
 * function instead `devfs_makedev`. */
int devfs_makedev_old(devfs_node_t *parent, const char *name, vnodeops_t *vops,
                      void *data, vnode_t **vnode_p);

/* TODO: remove it after rewriting drivers. */
void *devfs_node_data_old(vnode_t *vnode);

static inline void *devfs_node_data(devfs_node_t *dn) {
  return dn->dn_data;
}

/*
 * Remove a node from the devfs tree.
 * The devfs node and the corresponding vnode will no longer be accessible, but
 * there may still be existing references to them. The device driver should
 * provide a `d_reclaim()` function so that it is notified when it is safe to
 * free any driver-private data (the corresponding devfs node is released
 * automatically).
 * This function should be called form the driver's detach function.
 */
int devfs_unlink(devfs_node_t *dn);

/* TODO: rmove it after rewriting drivers. */
void devfs_free(devfs_node_t *dn);

#endif /* !_SYS_DEVFS_H_ */
