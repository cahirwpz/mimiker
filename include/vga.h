#ifndef _SYS_VGA_H_
#define _SYS_VGA_H_

#include <common.h>
#include <uio.h>

typedef struct vga_device vga_device_t;

typedef void (*vga_palette_write_t)(vga_device_t *vga,
                                    const uint8_t buf[3 * 256]);
typedef int (*vga_fb_write_t)(vga_device_t *vga, uio_t *uio);

typedef struct vga_device {
  vga_palette_write_t palette_write;
  vga_fb_write_t fb_write;
} vga_device_t;

/* Normally this function wouldn't need to exist, as PCI driver attachment would
   be part of PCI device discovery process. */
void vga_init();

/* Creates a new /dev/vga entry, linked to the vga device. */
void dev_vga_install(vga_device_t *vga);

static inline int vga_fb_write(vga_device_t *vga, uio_t *uio) {
  return vga->fb_write(vga, uio);
}
static inline void vga_palette_write(vga_device_t *vga,
                                     const uint8_t buf[3 * 256]) {
  vga->palette_write(vga, buf);
}

#endif /* _SYS_VGA_H_ */
