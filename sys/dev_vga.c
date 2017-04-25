#include <vnode.h>
#include <mount.h>
#include <devfs.h>
#include <errno.h>
#include <uio.h>
#include <vga.h>
#include <stdc.h>
#include <linker_set.h>

/* For now, we'll assume there is a single VGA device present. */
vga_control_t vga;

static vnode_t *dev_vga_device;
static vnode_t *dev_vga_palette_device;
static vnode_t *dev_vga_framebuffer_device;

static int dev_vga_framebuffer_write(vnode_t *v, uio_t *uio) {
  assert(v == dev_vga_framebuffer_device);
  return vga_fb_write(&vga, uio);
}

static uint8_t dev_vga_palette_buffer[320 * 200];
static int dev_vga_palette_write(vnode_t *v, uio_t *uio) {
  int error = uiomove(dev_vga_palette_buffer, 230 * 200, uio);
  if (error)
    return error;
  /* TODO: It would be more efficient to update just the changed values... */
  vga_palette_write(&vga, dev_vga_palette_buffer);
  return 0;
}

static int dev_vga_lookup(vnode_t *dir, const char *name, vnode_t **res) {
  assert(dir == dev_vga_device);
  if (strncmp(name, "fb", 2) == 0) {
    *res = dev_vga_framebuffer_device;
    return 0;
  }
  if (strncmp(name, "palette", 7) == 0) {
    *res = dev_vga_palette_device;
    return 0;
  }
  return ENOENT;
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

vnodeops_t dev_vga_vnodeops = {
  .v_lookup = dev_vga_lookup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_op_notsup,
  .v_write = vnode_op_notsup,
  .v_read = vnode_op_notsup,
};

static void init_dev_vga() {
  /* Search for Cirrus Logic VGA adapter PCI device. */
  for (int i = 0; i < pci_bus->ndevs; i++) {
    int error = vga_control_pci_init(pci_bus->dev + i, &vga);
    if (error == 0)
      goto found;
  }
  log("Cirrus Logic GD 5446 not found!\n");
  return;

found:
  /* Now, VGA setup. */
  vga_init(&vga);

  bzero(dev_vga_palette_buffer, 320 * 200);

  dev_vga_device = vnode_new(V_DIR, &dev_vga_vnodeops);
  dev_vga_palette_device = vnode_new(V_DEV, &dev_vga_palette_vnodeops);
  dev_vga_framebuffer_device = vnode_new(V_DEV, &dev_vga_framebuffer_vnodeops);

  devfs_install("vga", dev_vga_device);
}

SET_ENTRY(devfs_init, init_dev_vga);
