#ifndef _SYS_VNODE_H_
#define _SYS_VNODE_H_

#include <mutex.h>
#include <uio.h>

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
typedef int vnode_readdir_t(vnode_t *dv, uio_t *uio);
typedef int vnode_open_t(vnode_t *v, file_t **fp);
typedef int vnode_read_t(vnode_t *v, uio_t *uio);
typedef int vnode_write_t(vnode_t *v, uio_t *uio);
typedef int vnode_getattr_t(vnode_t *v, vattr_t *va);

typedef struct vnodeops {
  vnode_lookup_t *v_lookup;
  vnode_readdir_t *v_readdir;
  vnode_open_t *v_open;
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

typedef struct vattr {
  uint16_t va_mode; /* files access mode and type */
  size_t va_nlink;  /* number of references to file */
  uid_t va_uid;     /* owner user id */
  gid_t va_gid;     /* owner group id */
  size_t va_size;   /* file size in bytes */
} vattr_t;

static inline int VOP_LOOKUP(vnode_t *dv, const char *name, vnode_t **vp) {
  return dv->v_ops->v_lookup(dv, name, vp);
}

static inline int VOP_READDIR(vnode_t *dv, uio_t *uio) {
  return dv->v_ops->v_readdir(dv, uio);
}

static inline int VOP_OPEN(vnode_t *v, file_t **fp) {
  return v->v_ops->v_open(v, fp);
}

static inline int VOP_READ(vnode_t *v, uio_t *uio) {
  return v->v_ops->v_read(v, uio);
}

static inline int VOP_WRITE(vnode_t *v, uio_t *uio) {
  return v->v_ops->v_write(v, uio);
}

static inline int VOP_GETATTR(vnode_t *v, vattr_t *va) {
  return v->v_ops->v_getattr(v, va);
}

/* Initializes vnode subsystem */
void vnode_init();

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

#endif /* !_SYS_VNODE_H_ */
