#define KL_LOG KL_VFS
#include <klog.h>
#include <errno.h>
#include <file.h>
#include <pool.h>
#include <mutex.h>
#include <stdc.h>
#include <stat.h>
#include <vnode.h>

static POOL_DEFINE(P_VNODE, "vnode", sizeof(vnode_t));

/* Actually, vnode management should be much more complex than this, because
   this stub does not recycle vnodes, does not store them on a free list,
   etc. So at some point we may need a more sophisticated memory management here
   - but this will do for now. */

vnode_t *vnode_new(vnodetype_t type, vnodeops_t *ops, void *data) {
  vnode_t *v = pool_alloc(P_VNODE, PF_ZERO);
  v->v_type = type;
  v->v_data = data;
  v->v_ops = ops;
  v->v_usecnt = 1;
  mtx_init(&v->v_mtx, MTX_DEF);

  return v;
}

void vnode_lock(vnode_t *v) {
  mtx_lock(&v->v_mtx);
}

void vnode_unlock(vnode_t *v) {
  mtx_unlock(&v->v_mtx);
}

void vnode_hold(vnode_t *v) {
  refcnt_acquire(&v->v_usecnt);
}

void vnode_drop(vnode_t *v) {
  if (refcnt_release(&v->v_usecnt))
    pool_free(P_VNODE, v);
}

static int vnode_lookup_nop(vnode_t *dv, const char *name, vnode_t **vp) {
  return -ENOTSUP;
}

static int vnode_readdir_nop(vnode_t *dv, uio_t *uio, void *state) {
  return -ENOTSUP;
}

static int vnode_open_nop(vnode_t *v, int mode, file_t *fp) {
  return -ENOTSUP;
}

static int vnode_close_nop(vnode_t *v, file_t *fp) {
  return -ENOTSUP;
}

static int vnode_read_nop(vnode_t *v, uio_t *uio) {
  return -ENOTSUP;
}

static int vnode_write_nop(vnode_t *v, uio_t *uio) {
  return -ENOTSUP;
}

static int vnode_seek_nop(vnode_t *v, off_t oldoff, off_t newoff, void *state) {
  return -ENOTSUP;
}

static int vnode_getattr_nop(vnode_t *v, vattr_t *va) {
  *va = (vattr_t){.va_mode = VNOVAL,
                  .va_nlink = VNOVAL,
                  .va_uid = VNOVAL,
                  .va_gid = VNOVAL,
                  .va_size = VNOVAL};
  return 0;
}

int vnode_create_nop(vnode_t *dv, const char *name, vattr_t *va, vnode_t **vp) {
  return -ENOTSUP;
}

static int vnode_remove_nop(vnode_t *dv, const char *name) {
  return -ENOTSUP;
}

static int vnode_mkdir_nop(vnode_t *v, const char *name, vattr_t *va,
                           vnode_t **vp) {
  return -ENOTSUP;
}

static int vnode_rmdir_nop(vnode_t *v, const char *name) {
  return -ENOTSUP;
}

static int vnode_access_nop(vnode_t *v, accmode_t mode) {
  return -ENOTSUP;
}

#define NOP_IF_NULL(vops, name)                                                \
  do {                                                                         \
    if (vops->v_##name == NULL)                                                \
      vops->v_##name = vnode_##name##_nop;                                     \
  } while (0)

void vnodeops_init(vnodeops_t *vops) {
  NOP_IF_NULL(vops, lookup);
  NOP_IF_NULL(vops, readdir);
  NOP_IF_NULL(vops, open);
  NOP_IF_NULL(vops, close);
  NOP_IF_NULL(vops, read);
  NOP_IF_NULL(vops, write);
  NOP_IF_NULL(vops, seek);
  NOP_IF_NULL(vops, getattr);
  NOP_IF_NULL(vops, create);
  NOP_IF_NULL(vops, remove);
  NOP_IF_NULL(vops, mkdir);
  NOP_IF_NULL(vops, rmdir);
  NOP_IF_NULL(vops, access);
}

void va_convert(vattr_t *va, stat_t *sb) {
  memset(sb, 0, sizeof(stat_t));
  sb->st_mode = va->va_mode;
  sb->st_nlink = va->va_nlink;
  sb->st_uid = va->va_uid;
  sb->st_gid = va->va_gid;
  sb->st_size = va->va_size;
}

/* Default file operations using v-nodes. */
static int default_vnread(file_t *f, thread_t *td, uio_t *uio) {
  return VOP_READ(f->f_vnode, uio);
}

static int default_vnwrite(file_t *f, thread_t *td, uio_t *uio) {
  return VOP_WRITE(f->f_vnode, uio);
}

static int default_vnclose(file_t *f, thread_t *td) {
  (void)VOP_CLOSE(f->f_vnode, f);
  vnode_drop(f->f_vnode);
  return 0;
}

static int default_vnstat(file_t *f, thread_t *td, stat_t *sb) {
  vnode_t *v = f->f_vnode;
  vattr_t va;
  int error;
  error = VOP_GETATTR(v, &va);
  if (error < 0)
    return error;
  va_convert(&va, sb);
  return 0;
}

static int default_vnseek(file_t *f, thread_t *td, off_t offset, int whence) {
  vnode_t *v = f->f_vnode;
  int error;

  vattr_t va;
  if ((error = VOP_GETATTR(v, &va)))
    return -EINVAL;

  off_t size = va.va_size;

  if (size == VNOVAL)
    return -ESPIPE;

  if (whence == SEEK_CUR) {
    /* TODO: offset overflow */
    offset += f->f_offset;
  } else if (whence == SEEK_END) {
    /* TODO: offset overflow */
    offset += size;
  } else if (whence != SEEK_SET) {
    return -EINVAL;
  }

  if (offset < 0)
    return -EINVAL;

  /* TODO offset can go past the end of file when it's open for writing */
  if (offset > size)
    return -EINVAL;

  if ((error = VOP_SEEK(v, f->f_offset, offset, f->f_data)))
    return error;

  f->f_offset = offset;
  return offset;
}

static fileops_t default_vnode_fileops = {
  .fo_read = default_vnread,
  .fo_write = default_vnwrite,
  .fo_close = default_vnclose,
  .fo_seek = default_vnseek,
  .fo_stat = default_vnstat,
};

int vnode_open_generic(vnode_t *v, int mode, file_t *fp) {
  vnode_hold(v);
  fp->f_ops = &default_vnode_fileops;
  fp->f_type = FT_VNODE;
  fp->f_vnode = v;
  switch (mode) {
    case O_RDONLY:
      fp->f_flags = FF_READ;
      break;
    case O_WRONLY:
      fp->f_flags = FF_WRITE;
      break;
    case O_RDWR:
      fp->f_flags = FF_READ | FF_WRITE;
      break;
    default:
      return -EINVAL;
  }
  return 0;
}

int vnode_seek_generic(vnode_t *v, off_t oldoff, off_t newoff, void *state) {
  /* Operation went ok, assuming the file is seekable. */
  return 0;
}

int vnode_access_generic(vnode_t *v, accmode_t acc) {
  vattr_t va;
  int error;

  if ((error = VOP_GETATTR(v, &va)))
    return error;

  mode_t mode = 0;

  if (acc & VEXEC)
    mode |= S_IXUSR;
  if (acc & VWRITE)
    mode |= S_IWUSR;
  if (acc & VREAD)
    mode |= S_IRUSR;

  return ((va.va_mode & mode) == mode || acc == 0) ? 0 : -EACCES;
}
