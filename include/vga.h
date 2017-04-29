#ifndef _SYS_VGA_H_
#define _SYS_VGA_H_

#include <common.h>
#include <uio.h>

typedef struct vga_device vga_device_t;

typedef int (*vga_palette_write_t)(vga_device_t *vga, uio_t *uio);
typedef int (*vga_fb_write_t)(vga_device_t *vga, uio_t *uio);
typedef int (*vga_get_resolution_t)(vga_device_t *vga, uint16_t *xres,
                                    uint16_t *yres, unsigned int *bpp);
typedef int (*vga_set_resolution_t)(vga_device_t *vga, uint16_t xres,
                                    uint16_t yres, unsigned int bpp);

typedef struct vga_device {
  vga_palette_write_t palette_write;
  vga_fb_write_t fb_write;
  vga_get_resolution_t get_resolution;
  vga_set_resolution_t set_resolution;
} vga_device_t;

/* Normally this function wouldn't need to exist, as PCI driver attachment would
   be part of PCI device discovery process. */
void vga_init();

/* Creates a new /dev/vga entry, linked to the vga device. */
void dev_vga_install(vga_device_t *vga);

static inline int vga_fb_write(vga_device_t *vga, uio_t *uio) {
  return vga->fb_write(vga, uio);
}
static inline int vga_palette_write(vga_device_t *vga, uio_t *uio) {
  return vga->palette_write(vga, uio);
}
static inline int vga_get_resolution(vga_device_t *vga, uint16_t *xres,
                                     uint16_t *yres, unsigned int *bpp) {
  return vga->get_resolution(vga, xres, yres, bpp);
}
static inline int vga_set_resolution(vga_device_t *vga, uint16_t xres,
                                     uint16_t yres, unsigned int bpp) {
  return vga->set_resolution(vga, xres, yres, bpp);
}

#endif /* _SYS_VGA_H_ */
