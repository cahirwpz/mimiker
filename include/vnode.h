#ifndef _SYS_VNODE_H_
#define _SYS_VNODE_H_

#include <mutex.h>
#include <uio.h>
#include <malloc.h>

MALLOC_DECLARE(M_VFS);

/* Forward declarations */
typedef struct vnode vnode_t;
typedef struct vattr vattr_t;
typedef struct mount mount_t;
typedef struct file file_t;

typedef enum {
  V_NONE,
  V_REG,
  V_DIR,
  V_DEV,
} vnodetype_t;

typedef int vnode_lookup_t(vnode_t *dv, const char *name, vnode_t **vp);
typedef int vnode_readdir_t(vnode_t *dv, uio_t *uio, void *data);
typedef int vnode_open_t(vnode_t *v, int mode, file_t *fp);
typedef int vnode_close_t(vnode_t *v, file_t *fp);
typedef int vnode_read_t(vnode_t *v, uio_t *uio);
typedef int vnode_write_t(vnode_t *v, uio_t *uio);
typedef int vnode_getattr_t(vnode_t *v, vattr_t *va);

typedef struct vnodeops {
  vnode_lookup_t *v_lookup;
  vnode_readdir_t *v_readdir;
  vnode_open_t *v_open;
  vnode_close_t *v_close;
  vnode_read_t *v_read;
  vnode_write_t *v_write;
  vnode_getattr_t *v_getattr;
} vnodeops_t;

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

  int v_usecnt;
  mtx_t v_mtx;
} vnode_t;

static inline bool is_mountpoint(vnode_t *v) {
  return v->v_mountedhere != NULL;
}

#if !defined(IGNORE_NEWLIB_COMPATIBILITY)
/* This must match newlib's implementation! */
typedef struct vattr {
  dev_t st_dev;
  ino_t st_ino;
  mode_t st_mode;
  nlink_t st_nlink;
  uid_t st_uid;
  gid_t st_gid;
  dev_t st_rdev;
  off_t st_size;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
} vattr_t;
#else
typedef struct vattr {
  uint16_t va_mode; /* files access mode and type */
  size_t va_nlink;  /* number of references to file */
  uid_t va_uid;     /* owner user id */
  gid_t va_gid;     /* owner group id */
  size_t va_size;   /* file size in bytes */
} vattr_t;
#endif

#define VOP_CALL(op, vp, ...) ((vp)->v_ops->v_##op)((vp), __VA_ARGS__)

static inline int VOP_LOOKUP(vnode_t *dv, const char *name, vnode_t **vp) {
  return VOP_CALL(lookup, dv, name, vp);
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

static inline int VOP_GETATTR(vnode_t *v, vattr_t *va) {
  return VOP_CALL(getattr, v, va);
}

/* Allocates and initializes a new vnode */
vnode_t *vnode_new(vnodetype_t type, vnodeops_t *ops);

/* Lock and unlock vnode's mutex.
 * Call vnode_lock whenever you're about to use vnode's contents. */
void vnode_lock(vnode_t *v);
void vnode_unlock(vnode_t *v);

/* Increase and decrease the use counter.
 * Call vnode_ref if you don't want the vnode to be recycled. */
void vnode_ref(vnode_t *v);
void vnode_unref(vnode_t *v);

/* Convenience function for filling in not supported vnodeops */
int vnode_op_notsup();

int vnode_open_generic(vnode_t *v, int mode, file_t *fp);
int vnode_close_generic(vnode_t *v, file_t *fp);

#endif /* !_SYS_VNODE_H_ */
