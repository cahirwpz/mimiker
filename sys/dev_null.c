#include <vnode.h>
#include <mount.h>
#include <devfs.h>
#include <physmem.h>
#include <vnode.h>
#include <linker_set.h>

static vm_page_t *zero_page, *junk_page;

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
    error = uiomove(PG_KSEG0_ADDR(junk_page), len, uio);
  }
  return error;
}

static int dev_zero_read(vnode_t *v, uio_t *uio) {
  int error = 0;
  while (uio->uio_resid && !error) {
    size_t len = uio->uio_resid;
    if (len > PAGESIZE)
      len = PAGESIZE;
    error = uiomove(PG_KSEG0_ADDR(zero_page), len, uio);
  }
  return error;
}

static vnodeops_t dev_null_vnodeops = {.v_open = vnode_open_generic,
                                       .v_read = dev_null_read,
                                       .v_write = dev_null_write};

static vnodeops_t dev_zero_vnodeops = {.v_open = vnode_open_generic,
                                       .v_read = dev_zero_read,
                                       .v_write = dev_zero_write};

static void init_dev_null(void) {
  zero_page = pm_alloc(1);
  junk_page = pm_alloc(1);

  vnodeops_init(&dev_null_vnodeops);
  devfs_makedev(NULL, "null", &dev_null_vnodeops, NULL);

  vnodeops_init(&dev_zero_vnodeops);
  devfs_makedev(NULL, "zero", &dev_zero_vnodeops, NULL);
}

SET_ENTRY(devfs_init, init_dev_null);
