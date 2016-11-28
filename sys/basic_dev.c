#include <basic_dev.h>
#include <errno.h>
#include <filedesc.h>
#include <stdc.h>

static int devnull_read(file_t *f, struct thread *td, char *buf, size_t count) {
  /* For /dev/null such flag tests are redundant. Some other file providers may
     want to interpret these flags in their own way. */
  if (!(f->f_flags | FILE_FLAG_READ))
    return -EBADF;
  bzero(buf, count);
  return count;
}
static int devnull_write(file_t *f, struct thread *td, char *buf,
                         size_t count) {
  if (!(f->f_flags | FILE_FLAG_WRITE))
    return -EBADF;
  return count;
}
static int devnull_close(file_t *f, struct thread *td) {
  log("Closing a /dev/null");
  return 0;
}

static fileops_t devnull_ops = {
  .fo_read = devnull_read, .fo_write = devnull_write, .fo_close = devnull_close,
};

int dev_null_open(file_t *f, int flags, int mode) {
  log("Opening a /dev/null");
  f->f_data = NULL;
  f->f_type = FILE_TYPE_DEV;
  f->f_ops = &devnull_ops;
  /* Mode selection is trivial in case of a /dev/null */
  switch (mode) {
  case O_RDONLY:
    f->f_flags = FILE_FLAG_READ;
    break;
  case O_WRONLY:
    f->f_flags = FILE_FLAG_WRITE;
    break;
  case O_RDWR:
    f->f_flags = FILE_FLAG_READ | FILE_FLAG_WRITE;
    break;
  default:
    return -EINVAL;
  }
  return 0;
}
