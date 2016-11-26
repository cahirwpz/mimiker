#ifndef __FILE_H__
#define __FILE_H__

#include <stdint.h>
#include <stddef.h>

struct thread;
struct file;

typedef int fo_read_t(struct file *f, struct thread *td, char *buf,
                      size_t count);
typedef int fo_write_t(struct file *f, struct thread *td, char *buf,
                       size_t count);
typedef int fo_close_t(struct file *f, struct thread *td);

struct fileops {
  fo_read_t *fo_read;
  fo_write_t *fo_write;
  fo_close_t *fo_close;
};

typedef enum {
  FTYPE_VNODE = 1, /* regualar file */
  FTYPE_PIPE = 2,  /* pipe */
  FTYPE_DEV = 3,   /* device specific */
} file_type_t;

#define FREAD 0x0001
#define FWRITE 0x0002
#define FAPPEND 0x0004

struct file {
  void *f_data; /* file descriptor specific data */
  struct fileops f_ops;
  file_type_t f_type; /* file type */
  uint32_t f_count;   /* reference count */
  uint32_t f_flag;    /* F* flags */
};

#endif /* __FILE_H__ */
