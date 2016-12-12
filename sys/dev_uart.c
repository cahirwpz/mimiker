#include <vnode.h>
#include <mount.h>
#include <devfs.h>
#include <errno.h>
#include <uio.h>
#include <mips/uart_cbus.h>
#include <linker_set.h>

static vnode_t *dev_uart_device;

#define UART_BUF_MAX 100

static int dev_uart_write(vnode_t *t, uio_t *uio) {
  char buffer[UART_BUF_MAX];
  size_t n = uio->uio_resid;
  int res = uiomove(buffer, UART_BUF_MAX - 1, uio);
  if (res < 0)
    return res;
  size_t moved = n - uio->uio_resid;
  uart_write(buffer, moved);
  return moved;
}

static int dev_uart_read(vnode_t *t, uio_t *uio) {
  return ENOTSUP;
}

vnodeops_t dev_uart_vnodeops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_op_notsup,
  .v_write = dev_uart_write,
  .v_read = dev_uart_read,
};

void init_dev_uart() {
  dev_uart_device = vnode_new(V_DEV, &dev_uart_vnodeops);
  devfs_install("uart", dev_uart_device);
}

SET_ENTRY(devfs_init, init_dev_uart);
