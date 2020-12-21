#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/devfs.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/libkern.h>
#include <sys/vga.h>

static int framebuffer_write(vnode_t *v, uio_t *uio, int ioflag) {
  return vga_fb_write(devfs_node_data(v), uio);
}

static vnodeops_t framebuffer_vnodeops = {.v_write = framebuffer_write};

static int palette_write(vnode_t *v, uio_t *uio, int ioflag) {
  return vga_palette_write(devfs_node_data(v), uio);
}

static vnodeops_t palette_vnodeops = {.v_write = palette_write};

#define RES_CTRL_BUFFER_SIZE 16

static int videomode_write(vnode_t *v, uio_t *uio, int ioflag) {
  vga_device_t *vga = devfs_node_data(v);
  uio->uio_offset = 0; /* This file does not support offsets. */
  unsigned xres, yres, bpp;
  int error = vga_get_videomode(vga, &xres, &yres, &bpp);
  if (error)
    return error;
  /* Allocate one more byte since sscanf expects the buffer to be
   * NULL-terminated */
  char buffer[RES_CTRL_BUFFER_SIZE + 1];
  buffer[RES_CTRL_BUFFER_SIZE] = 0;
  error = uiomove_frombuf(buffer, RES_CTRL_BUFFER_SIZE, uio);
  if (error)
    return error;
  /* Not specifying BPP leaves it at current value. */
  int matches = sscanf(buffer, "%d %d %d", &xres, &yres, &bpp);
  if (matches < 2)
    return EINVAL;
  return vga_set_videomode(vga, xres, yres, bpp);
}

static int videomode_read(vnode_t *v, uio_t *uio, int ioflag) {
  vga_device_t *vga = devfs_node_data(v);
  unsigned xres, yres, bpp;
  int error;
  if ((error = vga_get_videomode(vga, &xres, &yres, &bpp)))
    return error;
  char buffer[RES_CTRL_BUFFER_SIZE];
  (void)snprintf(buffer, RES_CTRL_BUFFER_SIZE, "%d %d %d", xres, yres, bpp);
  if (error >= RES_CTRL_BUFFER_SIZE)
    return EINVAL;
  return uiomove_frombuf(buffer, RES_CTRL_BUFFER_SIZE, uio);
}

static vnodeops_t videomode_vnodeops = {.v_read = videomode_read,
                                        .v_write = videomode_write};

void dev_vga_install(vga_device_t *vga) {
  devfs_node_t *vga_root;

  /* Only a single vga device may be installed at /dev/vga. */
  if (devfs_makedir(NULL, "vga", &vga_root))
    return;

  devfs_makedev(vga_root, "fb", &framebuffer_vnodeops, vga, NULL);
  devfs_makedev(vga_root, "palette", &palette_vnodeops, vga, NULL);
  devfs_makedev(vga_root, "videomode", &videomode_vnodeops, vga, NULL);
}
