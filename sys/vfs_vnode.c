#include <errno.h>
#include <file.h>
#include <malloc.h>
#include <mutex.h>
#include <stdc.h>
#include <stat.h>
#include <vnode.h>
#include <sysinit.h>

static MALLOC_DEFINE(M_VNODE, "vnode", 2, 16);

/* Actually, vnode management should be much more complex than this, because
   this stub does not recycle vnodes, does not store them on a free list,
   etc. So at some point we may need a more sophisticated memory management here
   - but this will do for now. */

static void vnode_init(void) {
}

vnode_t *vnode_new(vnodetype_t type, vnodeops_t *ops) {
  vnode_t *v = kmalloc(M_VNODE, sizeof(vnode_t), M_ZERO);
  v->v_type = type;
  v->v_data = NULL;
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

void vnode_ref(vnode_t *v) {
  vnode_lock(v);
  v->v_usecnt++;
  vnode_unlock(v);
}

void vnode_unref(vnode_t *v) {
  vnode_lock(v);
  v->v_usecnt--;
  if (v->v_usecnt == 0)
    kfree(M_VNODE, v);
  else
    vnode_unlock(v);
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
  return -ENOTSUP;
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
  vnode_unref(f->f_vnode);
  return 0;
}

static int default_vnstat(file_t *f, thread_t *td, stat_t *sb) {
  vnode_t *v = f->f_vnode;
  vattr_t va;
  int error;
  error = VOP_GETATTR(v, &va);
  if (error < 0)
    return error;
  memset(sb, 0, sizeof(stat_t));
  sb->st_mode = va.va_mode;
  sb->st_nlink = va.va_nlink;
  sb->st_uid = va.va_uid;
  sb->st_gid = va.va_gid;
  sb->st_size = va.va_size;
  return 0;
}

static int default_vnseek(file_t *f, thread_t *td, off_t offset, int whence) {
  /* TODO: Whence! Now we assume whence == SEEK_SET */
  if (whence)
    return -EINVAL;
  /* TODO: file cursor must be within file, i.e. [0, vattr.v_size] */
  if (offset < 0)
    return -EINVAL;
  int error = VOP_SEEK(f->f_vnode, f->f_offset, offset, f->f_data);
  if (error)
    return error;
  f->f_offset = offset;
  return 0;
}

static fileops_t default_vnode_fileops = {
  .fo_read = default_vnread,
  .fo_write = default_vnwrite,
  .fo_close = default_vnclose,
  .fo_seek = default_vnseek,
  .fo_stat = default_vnstat,
};

int vnode_open_generic(vnode_t *v, int mode, file_t *fp) {
  vnode_ref(v);
  fp->f_ops = &default_vnode_fileops;
  fp->f_type = FT_VNODE;
  fp->f_vnode = v;
  switch (mode & O_RDWR) {
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

SYSINIT_ADD(vnode, vnode_init, DEPS("vm_map"));
