#ifndef _SYS_FILE_H_
#define _SYS_FILE_H_

#include <stdint.h>
#include <stddef.h>
#include <mutex.h>
#include <uio.h>

typedef struct thread thread_t;
typedef struct file file_t;
typedef struct vnode vnode_t;

typedef int fo_read_t(file_t *f, thread_t *td, uio_t *uio);
typedef int fo_write_t(file_t *f, thread_t *td, uio_t *uio);
typedef int fo_close_t(file_t *f, thread_t *td);

typedef struct {
  fo_read_t *fo_read;
  fo_write_t *fo_write;
  fo_close_t *fo_close;
} fileops_t;

typedef enum {
  FT_VNODE = 1, /* regular file */
  FT_PIPE = 2,  /* pipe */
} filetype_t;

#define FF_READ 0x0001
#define FF_WRITE 0x0002
#define FF_APPEND 0x0004

/* File open modes as passed to sys_open. These need match what newlib provides
   to user programs. */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

typedef struct file {
  void *f_data; /* File specific data */
  fileops_t *f_ops;
  filetype_t f_type; /* File type */
  vnode_t *f_vnode;
  uint32_t f_count; /* Reference count */
  uint32_t f_flags; /* FILE_FLAG_* */
  mtx_t f_mtx;
} file_t;

void file_ref(file_t *f);
void file_unref(file_t *f);


static inline int FOP_READ(file_t *f, thread_t *td, uio_t *uio) {
  return f->f_ops->fo_read(f, td, uio);
}

static inline int FOP_WRITE(file_t *f, thread_t *td, uio_t *uio) {
  return f->f_ops->fo_write(f, td, uio);
}

static inline int FOP_CLOSE(file_t *f, thread_t *td) {
  return f->f_ops->fo_close(f, td);
}

#endif /* !_SYS_FILE_H_ */
