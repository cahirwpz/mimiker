#include <vnode.h>
#include <mount.h>
#include <devfs.h>
#include <errno.h>
#include <uio.h>
#include <stdc.h>
#include <vga.h>

static int dev_vga_framebuffer_write(vnode_t *v, uio_t *uio) {
  vga_device_t *vga = (vga_device_t *)v->v_data;
  return vga_fb_write(vga, uio);
}

static int dev_vga_palette_write(vnode_t *v, uio_t *uio) {
  vga_device_t *vga = (vga_device_t *)v->v_data;
  return vga_palette_write(vga, uio);
}

vnodeops_t dev_vga_framebuffer_vnodeops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_open_generic,
  .v_write = dev_vga_framebuffer_write,
  .v_read = vnode_op_notsup,
};

vnodeops_t dev_vga_palette_vnodeops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_open_generic,
  .v_write = dev_vga_palette_write,
  .v_read = vnode_op_notsup,
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
  return ENOENT;
}

vnodeops_t dev_vga_vnodeops = {
  .v_lookup = dev_vga_lookup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_op_notsup,
  .v_write = vnode_op_notsup,
  .v_read = vnode_op_notsup,
};

void dev_vga_install(vga_device_t *vga) {
  vnode_t *dev_vga_device = vnode_new(V_DIR, &dev_vga_vnodeops);
  dev_vga_device->v_data = vga;
  devfs_install("vga", dev_vga_device);
}
