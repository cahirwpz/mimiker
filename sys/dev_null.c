#include <cdev.h>
#include <malloc.h>
#include <devfs.h>

static cdev_t *dev_null, *dev_zero;
vm_page_t *zero_page, *junk_page;

int dev_null_open(cdev_t *dev, int flags) {
  return 0;
}
int dev_null_close(cdev_t *dev) {
  return 0;
}
int dev_null_read(cdev_t *dev, uio_t *uio, int flags) {
  return 0;
}
int dev_null_write(cdev_t *dev, uio_t *uio, int flags) {
  uio->uio_resid = 0;
  return 0;
}

int dev_zero_read(cdev_t *dev, uio_t *uio, int flags) {
  int error = 0;
  while (uio->uio_resid && !error) {
    size_t len = uio->uio_resid;
    if (len > PAGESIZE)
      len = PAGESIZE;
    error = uiomove((void *)zero_page->vaddr, len, uio);
  }
  return error;
}

int dev_zero_write(cdev_t *dev, uio_t *uio, int flags) {
  /* We might just discard the data, but to demonstrate using uiomove for
   * writing, store the data into a junkyard page. */
  int error = 0;
  while (uio->uio_resid && !error) {
    size_t len = uio->uio_resid;
    if (len > PAGESIZE)
      len = PAGESIZE;
    error = uiomove((void *)junk_page->vaddr, len, uio);
  }
  return error;
}

static cdevsw_t dev_null_sw = {.d_open = dev_null_open,
                               .d_close = dev_null_close,
                               .d_read = dev_null_read,
                               .d_write = dev_null_write};

static cdevsw_t dev_zero_sw = {.d_open = dev_null_open,
                               .d_close = dev_null_close,
                               .d_read = dev_zero_read,
                               .d_write = dev_zero_write};

void dev_null_init() {
  zero_page = pm_alloc(1);
  junk_page = pm_alloc(1);
  dev_null = make_dev(dev_null_sw, "null");
  dev_zero = make_dev(dev_zero_sw, "zero");
}
