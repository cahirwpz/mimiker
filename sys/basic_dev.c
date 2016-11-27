#include <basic_dev.h>
#include <errno.h>
#include <filedesc.h>
#include <stdc.h>

static int devnull_read(file_t *f, struct thread *td, char *buf, size_t count) {
  bzero(buf, count);
  return count;
}
static int devnull_write(file_t *f, struct thread *td, char *buf,
                         size_t count) {
  return count;
}
static int devnull_close(file_t *f, struct thread *td) {
  return 0;
}

fileops_t devnull_ops = {
  .fo_read = devnull_read, .fo_write = devnull_write, .fo_close = devnull_close,
};

int dev_null_open(file_t *f, int flags, int mode) {
  f->f_data = NULL;
  f->f_type = FTYPE_DEV;
  f->f_ops = devnull_ops;
  return 0;
}
