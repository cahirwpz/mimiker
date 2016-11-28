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
  FILE_TYPE_VNODE = 1, /* regualar file */
  FILE_TYPE_PIPE = 2,  /* pipe */
  FILE_TYPE_DEV = 3,   /* device specific */
} file_type_t;

#define FILE_FLAG_READ 0x0001
#define FILE_FLAG_WRITE 0x0002
#define FILE_FLAG_APPEND 0x0004

/* File open modes as passed to sys_open. These need match what newlib provides
   to user programs. */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

typedef struct file {
  void *f_data; /* File specific data */
  fileops_t *f_ops;
  file_type_t f_type; /* File type */
  uint32_t f_count;   /* Reference count */
  uint32_t f_flags;   /* FILE_FLAG_* */
  mtx_t f_mtx;
} file_t;

void file_hold(file_t *f);
void file_drop(file_t *f);

#endif /* __FILE_H__ */
