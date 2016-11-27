#ifndef __FILE_H__
#define __FILE_H__

#include <stdint.h>
#include <stddef.h>
#include <mutex.h>

typedef struct thread thread_t;
typedef struct file file_t;

typedef int fo_read_t(file_t *f, thread_t *td, char *buf, size_t count);
typedef int fo_write_t(file_t *f, thread_t *td, char *buf, size_t count);
typedef int fo_close_t(file_t *f, thread_t *td);

typedef struct {
  fo_read_t *fo_read;
  fo_write_t *fo_write;
  fo_close_t *fo_close;
} fileops_t;

typedef enum {
  FTYPE_VNODE = 1, /* regualar file */
  FTYPE_PIPE = 2,  /* pipe */
  FTYPE_DEV = 3,   /* device specific */
} file_type_t;

#define FREAD 0x0001
#define FWRITE 0x0002
#define FAPPEND 0x0004

typedef struct file {
  void *f_data; /* file descriptor specific data */
  fileops_t *f_ops;
  file_type_t f_type; /* file type */
  uint32_t f_count;   /* reference count */
  uint32_t f_flag;    /* F* flags */
  mtx_t f_mtx;
} file_t;

void file_hold(file_t *f);
void file_drop(file_t *f);

#endif /* __FILE_H__ */
