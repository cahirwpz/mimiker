#ifndef _SYS_FILE_H_
#define _SYS_FILE_H_

#include <sys/cdefs.h>
#include <sys/mutex.h>
#include <sys/fcntl.h>
#include <sys/refcnt.h>

typedef struct file file_t;
typedef struct vnode vnode_t;
typedef struct stat stat_t;
typedef struct uio uio_t;

typedef int fo_read_t(file_t *f, uio_t *uio);
typedef int fo_write_t(file_t *f, uio_t *uio);
typedef int fo_close_t(file_t *f);
typedef int fo_seek_t(file_t *f, off_t offset, int whence);
typedef int fo_stat_t(file_t *f, stat_t *sb);

typedef struct {
  fo_read_t *fo_read;
  fo_write_t *fo_write;
  fo_close_t *fo_close;
  fo_seek_t *fo_seek;
  fo_stat_t *fo_stat;
} fileops_t;

typedef enum {
  FT_VNODE = 1, /* regular file */
  FT_PIPE = 2,  /* pipe */
} filetype_t;

#define FF_READ 0x0001
#define FF_WRITE 0x0002
#define FF_APPEND 0x0004

typedef struct file {
  void *f_data; /* File specific data */
  fileops_t *f_ops;
  filetype_t f_type; /* File type */
  vnode_t *f_vnode;
  off_t f_offset;
  refcnt_t f_count; /* Reference counter */
  unsigned f_flags; /* File flags FF_* */
  mtx_t f_mtx;
} file_t;

file_t *file_alloc(void);
void file_destroy(file_t *f);

/*! \brief Increments reference counter. */
void file_hold(file_t *f);

/*! \brief Decrements refcounter and destroys file if it has reached 0. */
void file_drop(file_t *f);

static inline int FOP_READ(file_t *f, uio_t *uio) {
  return f->f_ops->fo_read(f, uio);
}

static inline int FOP_WRITE(file_t *f, uio_t *uio) {
  return f->f_ops->fo_write(f, uio);
}

static inline int FOP_CLOSE(file_t *f) {
  return f->f_ops->fo_close(f);
}

static inline int FOP_SEEK(file_t *f, off_t offset, int whence) {
  return f->f_ops->fo_seek(f, offset, whence);
}

static inline int FOP_STAT(file_t *f, stat_t *sb) {
  return f->f_ops->fo_stat(f, sb);
}

extern fileops_t badfileops;

#endif /* !_SYS_FILE_H_ */
