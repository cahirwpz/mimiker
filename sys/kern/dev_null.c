#include <sys/devfs.h>
#include <sys/pmap.h>
#include <sys/kmem.h>
#include <sys/vnode.h>
#include <sys/linker_set.h>

static void *zero_page, *junk_page;

static int dev_null_write(vnode_t *v, uio_t *uio) {
  uio->uio_resid = 0;
  return 0;
}

static int dev_null_read(vnode_t *v, uio_t *uio) {
  return 0;
}

static int dev_zero_write(vnode_t *v, uio_t *uio) {
  /* We might just discard the data, but to demonstrate using uiomove for
   * writing, store the data into a junkyard page. */
  int error = 0;
  while (uio->uio_resid && !error) {
    size_t len = uio->uio_resid;
    if (len > PAGESIZE)
      len = PAGESIZE;
    error = uiomove(junk_page, len, uio);
  }
  return error;
}

static int dev_zero_read(vnode_t *v, uio_t *uio) {
  int error = 0;
  while (uio->uio_resid && !error) {
    size_t len = uio->uio_resid;
    if (len > PAGESIZE)
      len = PAGESIZE;
    error = uiomove(zero_page, len, uio);
  }
  return error;
}

static vnodeops_t dev_null_vnodeops = {.v_read = dev_null_read,
                                       .v_write = dev_null_write};

static vnodeops_t dev_zero_vnodeops = {.v_read = dev_zero_read,
                                       .v_write = dev_zero_write};

static void init_dev_null(void) {
  zero_page = kmem_alloc(PAGESIZE, M_ZERO);
  junk_page = kmem_alloc(PAGESIZE, 0);

  devfs_makedev_old(NULL, "null", &dev_null_vnodeops, NULL, NULL);
  devfs_makedev_old(NULL, "zero", &dev_zero_vnodeops, NULL, NULL);
}

SET_ENTRY(devfs_init, init_dev_null);
