#include <vnode.h>
#include <mount.h>
#include <devfs.h>
#include <errno.h>
#include <uio.h>
#include <console.h>
#include <linker_set.h>

static vnode_t *dev_cons_device;

#define UART_BUF_MAX 100

static int dev_cons_write(vnode_t *t, uio_t *uio) {
  char buffer[UART_BUF_MAX];
  size_t n = uio->uio_resid;
  int res = uiomove(buffer, UART_BUF_MAX - 1, uio);
  if (res)
    return res;
  size_t moved = n - uio->uio_resid;
  cn_write(buffer, moved);
  return 0;
}

static int dev_cons_read(vnode_t *t, uio_t *uio) {
  char buffer[UART_BUF_MAX];
  unsigned curr = 0;
  while (curr < UART_BUF_MAX && curr < uio->uio_resid) {
    buffer[curr] = cn_getc();
    if (buffer[curr++] == '\n')
      break;
  }
  uio->uio_offset = 0; /* This device does not support offsets. */
  int res = uiomove_frombuf(buffer, curr, uio);
  if (res)
    return res;
  return 0;
}

static vnodeops_t dev_cons_vnodeops = {
  .v_lookup = (vnode_lookup_t *)vnode_op_notsup,
  .v_readdir = (vnode_readdir_t *)vnode_op_notsup,
  .v_open = vnode_open_generic,
  .v_close = (vnode_close_t *)vnode_op_notsup,
  .v_read = dev_cons_read,
  .v_write = dev_cons_write,
  .v_seek = (vnode_seek_t *)vnode_op_notsup,
  .v_getattr = (vnode_getattr_t *)vnode_op_notsup};

static void init_dev_cons(void) {
  dev_cons_device = vnode_new(V_DEV, &dev_cons_vnodeops);
  devfs_install("cons", dev_cons_device);
}

SET_ENTRY(devfs_init, init_dev_cons);
