#ifndef _SYS_VNODE_H_
#define _SYS_VNODE_H_

#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/uio.h>
#include <sys/refcnt.h>

/* Forward declarations */
typedef struct vnode vnode_t;
typedef struct vattr vattr_t;
typedef struct mount mount_t;
typedef struct file file_t;
typedef struct dirent dirent_t;
typedef struct stat stat_t;
typedef struct componentname componentname_t;

#define VNOVAL (-1)

/* vnode access modes */
typedef enum { VEXEC = 1, VWRITE = 2, VREAD = 4 } accmode_t;

typedef enum {
  V_NONE,
  V_REG,
  V_DIR,
  V_DEV,
} vnodetype_t;

typedef int vnode_lookup_t(vnode_t *dv, componentname_t *cn, vnode_t **vp);
typedef int vnode_readdir_t(vnode_t *dv, uio_t *uio, void *state);
typedef int vnode_open_t(vnode_t *v, int mode, file_t *fp);
typedef int vnode_close_t(vnode_t *v, file_t *fp);
typedef int vnode_read_t(vnode_t *v, uio_t *uio);
typedef int vnode_write_t(vnode_t *v, uio_t *uio);
typedef int vnode_seek_t(vnode_t *v, off_t oldoff, off_t newoff, void *state);
typedef int vnode_getattr_t(vnode_t *v, vattr_t *va);
typedef int vnode_create_t(vnode_t *dv, const char *name, vattr_t *va,
                           vnode_t **vp);
typedef int vnode_remove_t(vnode_t *dv, const char *name);
typedef int vnode_mkdir_t(vnode_t *dv, const char *name, vattr_t *va,
                          vnode_t **vp);
typedef int vnode_rmdir_t(vnode_t *dv, const char *name);
typedef int vnode_access_t(vnode_t *v, accmode_t mode);
typedef int vnode_ioctl_t(vnode_t *v, u_long cmd, void *data);
typedef int vnode_reclaim_t(vnode_t *v);

typedef struct vnodeops {
  vnode_lookup_t *v_lookup;
  vnode_readdir_t *v_readdir;
  vnode_open_t *v_open;
  vnode_close_t *v_close;
  vnode_read_t *v_read;
  vnode_write_t *v_write;
  vnode_seek_t *v_seek;
  vnode_getattr_t *v_getattr;
  vnode_create_t *v_create;
  vnode_remove_t *v_remove;
  vnode_mkdir_t *v_mkdir;
  vnode_rmdir_t *v_rmdir;
  vnode_access_t *v_access;
  vnode_ioctl_t *v_ioctl;
  vnode_reclaim_t *v_reclaim;
} vnodeops_t;

/* Fill missing entries with default vnode operation. */
void vnodeops_init(vnodeops_t *vops);

typedef struct vnode {
  vnodetype_t v_type;        /* Vnode type, see above */
  TAILQ_ENTRY(vnode) v_list; /* Entry on the mount vnodes list */

  vnodeops_t *v_ops; /* Vnode operations */
  void *v_data;      /* Filesystem-specific arbitrary data */

  mount_t *v_mount; /* Pointer to the mount we are in */

  /* Type-specific fields */
  union {
    mount_t *v_mountedhere; /* The mount covering this vnode */
  };

  refcnt_t v_usecnt;
  mtx_t v_mtx;
} vnode_t;

static inline bool is_mountpoint(vnode_t *v) {
  return v->v_mountedhere != NULL;
}

typedef struct vattr {
  mode_t va_mode;   /* files access mode and type */
  nlink_t va_nlink; /* number of references to file */
  ino_t va_ino;     /* file id */
  uid_t va_uid;     /* owner user id */
  gid_t va_gid;     /* owner group id */
  size_t va_size;   /* file size in bytes */
} vattr_t;

void va_convert(vattr_t *va, stat_t *sb);

#define VOP_CALL(op, v, ...)                                                   \
  ((v)->v_ops->v_##op) ? ((v)->v_ops->v_##op(v, ##__VA_ARGS__)) : ENOTSUP

static inline int VOP_LOOKUP(vnode_t *dv, componentname_t *cn, vnode_t **vp) {
  return VOP_CALL(lookup, dv, cn, vp);
}

static inline int VOP_READDIR(vnode_t *dv, uio_t *uio, void *data) {
  return VOP_CALL(readdir, dv, uio, data);
}

static inline int VOP_OPEN(vnode_t *v, int mode, file_t *fp) {
  return VOP_CALL(open, v, mode, fp);
}

static inline int VOP_CLOSE(vnode_t *v, file_t *fp) {
  return VOP_CALL(close, v, fp);
}

static inline int VOP_READ(vnode_t *v, uio_t *uio) {
  return VOP_CALL(read, v, uio);
}

static inline int VOP_WRITE(vnode_t *v, uio_t *uio) {
  return VOP_CALL(write, v, uio);
}

static inline int VOP_SEEK(vnode_t *v, off_t oldoff, off_t newoff,
                           void *state) {
  return VOP_CALL(seek, v, oldoff, newoff, state);
}

static inline int VOP_GETATTR(vnode_t *v, vattr_t *va) {
  return VOP_CALL(getattr, v, va);
}

static inline int VOP_CREATE(vnode_t *dv, const char *name, vattr_t *va,
                             vnode_t **vp) {
  return VOP_CALL(create, dv, name, va, vp);
}

static inline int VOP_REMOVE(vnode_t *dv, const char *name) {
  return VOP_CALL(remove, dv, name);
}

static inline int VOP_MKDIR(vnode_t *dv, const char *name, vattr_t *va,
                            vnode_t **vp) {
  return VOP_CALL(mkdir, dv, name, va, vp);
}

static inline int VOP_RMDIR(vnode_t *dv, const char *name) {
  return VOP_CALL(rmdir, dv, name);
}

static inline int VOP_ACCESS(vnode_t *v, mode_t mode) {
  return VOP_CALL(access, v, mode);
}

static inline int VOP_IOCTL(vnode_t *v, u_long cmd, void *data) {
  return VOP_CALL(ioctl, v, cmd, data);
}

static inline int VOP_RECLAIM(vnode_t *v) {
  return VOP_CALL(reclaim, v);
}

#undef VOP_CALL

/* Allocates and initializes a new vnode */
vnode_t *vnode_new(vnodetype_t type, vnodeops_t *ops, void *data);

/* Lock and unlock vnode's mutex.
 * Call vnode_lock whenever you're about to use vnode's contents. */
void vnode_lock(vnode_t *v);
void vnode_unlock(vnode_t *v);

/* Increase and decrease the use counter.
 * Call vnode_ref if you don't want the vnode to be recycled. */
void vnode_hold(vnode_t *v);
void vnode_drop(vnode_t *v);

/* Convenience function with default vnode operation implementation. */
int vnode_open_generic(vnode_t *v, int mode, file_t *fp);
int vnode_seek_generic(vnode_t *v, off_t oldoff, off_t newoff, void *state);
int vnode_access_generic(vnode_t *v, accmode_t mode);

#define DIRENT_DOT ((void *)-2)
#define DIRENT_DOTDOT ((void *)-1)
#define DIRENT_EOF NULL

typedef struct readdir_ops {
  /* take next directory entry */
  void *(*next)(vnode_t *dir, void *entry);
  /* filename size (to calc. dirent size) */
  size_t (*namlen_of)(vnode_t *dir, void *entry);
  /* make dirent based on entry */
  void (*convert)(vnode_t *dir, void *entry, dirent_t *dirent);
} readdir_ops_t;

int readdir_generic(vnode_t *v, uio_t *uio, readdir_ops_t *ops);

#endif /* !_SYS_VNODE_H_ */
