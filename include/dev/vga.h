#ifndef _DEV_VGA_H_
#define _DEV_VGA_H_

#include <sys/cdefs.h>
#include <sys/uio.h>

typedef struct vga_device vga_device_t;

typedef int (*vga_palette_write_t)(vga_device_t *vga, uio_t *uio);
typedef int (*vga_fb_write_t)(vga_device_t *vga, uio_t *uio);
typedef int (*vga_get_videomode_t)(vga_device_t *vga, unsigned *xres,
                                   unsigned *yres, unsigned *bpp);
typedef int (*vga_set_videomode_t)(vga_device_t *vga, unsigned xres,
                                   unsigned yres, unsigned bpp);

typedef struct vga_device {
  vga_palette_write_t palette_write;
  vga_fb_write_t fb_write;
  vga_get_videomode_t get_videomode;
  vga_set_videomode_t set_videomode;
} vga_device_t;

/* Normally this function wouldn't need to exist, as PCI driver attachment would
   be part of PCI device discovery process. */
void vga_init(void);

/* Creates a new /dev/vga entry, linked to the vga device. */
void dev_vga_install(vga_device_t *vga);

static inline int vga_fb_write(vga_device_t *vga, uio_t *uio) {
  return vga->fb_write(vga, uio);
}
static inline int vga_palette_write(vga_device_t *vga, uio_t *uio) {
  return vga->palette_write(vga, uio);
}
static inline int vga_get_videomode(vga_device_t *vga, unsigned *xres,
                                    unsigned *yres, unsigned *bpp) {
  return vga->get_videomode(vga, xres, yres, bpp);
}
static inline int vga_set_videomode(vga_device_t *vga, unsigned xres,
                                    unsigned yres, unsigned bpp) {
  return vga->set_videomode(vga, xres, yres, bpp);
}

#endif /* !_DEV_VGA_H_ */
