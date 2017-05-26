#include <vnode.h>
#include <mount.h>
#include <devfs.h>
#include <errno.h>
#include <uio.h>
#include <stdc.h>
#include <vga.h>

static int dev_vga_framebuffer_write(vnode_t *v, uio_t *uio) {
  return vga_fb_write((vga_device_t *)v->v_data, uio);
}

static int dev_vga_palette_write(vnode_t *v, uio_t *uio) {
  return vga_palette_write((vga_device_t *)v->v_data, uio);
}

#define RES_CTRL_BUFFER_SIZE 16

static int dev_vga_videomode_write(vnode_t *v, uio_t *uio) {
  vga_device_t *vga = (vga_device_t *)v->v_data;
  uio->uio_offset = 0; /* This file does not support offsets. */
  unsigned xres, yres, bpp;
  int error = vga_get_videomode(vga, &xres, &yres, &bpp);
  if (error)
    return error;
  char buffer[RES_CTRL_BUFFER_SIZE];
  error = uiomove_frombuf(buffer, RES_CTRL_BUFFER_SIZE, uio);
  if (error)
    return error;
  /* Not specifying BPP leaves it at current value. */
  int matches = sscanf(buffer, "%d %d %d", &xres, &yres, &bpp);
  if (matches < 2)
    return -EINVAL;
  error = vga_set_videomode(vga, xres, yres, bpp);
  if (error)
    return error;
  return 0;
}

static int dev_vga_videomode_read(vnode_t *v, uio_t *uio) {
  vga_device_t *vga = (vga_device_t *)v->v_data;
  unsigned xres, yres, bpp;
  int error = vga_get_videomode(vga, &xres, &yres, &bpp);
  if (error)
    return error;
  char buffer[RES_CTRL_BUFFER_SIZE];
  error = snprintf(buffer, RES_CTRL_BUFFER_SIZE, "%d %d %d", xres, yres, bpp);
  if (error < 0)
    return error;
  if (error >= RES_CTRL_BUFFER_SIZE)
    return -EINVAL;
  error = uiomove_frombuf(buffer, RES_CTRL_BUFFER_SIZE, uio);
  if (error)
    return error;
  return 0;
}

static vnodeops_t dev_vga_framebuffer_vnodeops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_open_generic,
  .v_close = vnode_op_notsup,
  .v_getattr = vnode_op_notsup,
  .v_write = dev_vga_framebuffer_write,
  .v_read = vnode_op_notsup,
};

static vnodeops_t dev_vga_palette_vnodeops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_open_generic,
  .v_close = vnode_op_notsup,
  .v_getattr = vnode_op_notsup,
  .v_write = dev_vga_palette_write,
  .v_read = vnode_op_notsup,
};

static vnodeops_t dev_vga_videomode_vnodeops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_open_generic,
  .v_close = vnode_op_notsup,
  .v_getattr = vnode_op_notsup,
  .v_write = dev_vga_videomode_write,
  .v_read = dev_vga_videomode_read,
};

static int dev_vga_lookup(vnode_t *v, const char *name, vnode_t **res) {
  vga_device_t *vga = (vga_device_t *)v->v_data;
  if (strncmp(name, "fb", 2) == 0) {
    *res = vnode_new(V_DEV, &dev_vga_framebuffer_vnodeops);
    (*res)->v_data = vga;
    return 0;
  }
  if (strncmp(name, "palette", 7) == 0) {
    *res = vnode_new(V_DEV, &dev_vga_palette_vnodeops);
    (*res)->v_data = vga;
    return 0;
  }
  if (strncmp(name, "videomode", 7) == 0) {
    *res = vnode_new(V_DEV, &dev_vga_videomode_vnodeops);
    (*res)->v_data = vga;
    return 0;
  }
  return ENOENT;
}

static vnodeops_t dev_vga_vnodeops = {
  .v_lookup = dev_vga_lookup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_op_notsup,
  .v_close = vnode_op_notsup,
  .v_write = vnode_op_notsup,
  .v_read = vnode_op_notsup,
};

void dev_vga_install(vga_device_t *vga) {
  static int installed = 0;
  if (installed++) /* Only a single vga device may be installed at /dev/vga. */
    return;

  vnode_t *dev_vga_device = vnode_new(V_DIR, &dev_vga_vnodeops);
  dev_vga_device->v_data = vga;
  devfs_install("vga", dev_vga_device);
}
