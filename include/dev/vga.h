#ifndef _DEV_VGA_H_
#define _DEV_VGA_H_

#include <sys/cdefs.h>
#include <sys/uio.h>

typedef struct vga_device vga_device_t;
typedef struct fb_info fb_info_t;
typedef struct fb_palette fb_palette_t;

typedef int (*vga_palette_write_t)(vga_device_t *vga,
                                   fb_palette_t *user_palette);
typedef int (*vga_fb_write_t)(vga_device_t *vga, uio_t *uio);
typedef int (*vga_get_fbinfo_t)(vga_device_t *vga, fb_info_t *fbinfo);
typedef int (*vga_set_fbinfo_t)(vga_device_t *vga, fb_info_t *fbinfo);

typedef struct vga_ops {
  vga_palette_write_t palette_write;
  vga_fb_write_t fb_write;
  vga_get_fbinfo_t get_fbinfo;
  vga_set_fbinfo_t set_fbinfo;
} vga_ops_t;

typedef struct vga_device {
  atomic_int vg_usecnt;
  vga_ops_t *vg_ops;
} vga_device_t;

/* Normally this function wouldn't need to exist, as PCI driver attachment would
   be part of PCI device discovery process. */
void vga_init(void);

/* Creates a new /dev/vga entry, linked to the vga device. */
void dev_vga_install(vga_device_t *vga);

fb_palette_t *fb_palette_create(size_t len);
void fb_palette_destroy(fb_palette_t *palette);

static inline int vga_fb_write(vga_device_t *vga, uio_t *uio) {
  return vga->vg_ops->fb_write(vga, uio);
}
static inline int vga_palette_write(vga_device_t *vga, fb_palette_t *palette) {
  return vga->vg_ops->palette_write(vga, palette);
}
static inline int vga_get_fbinfo(vga_device_t *vga, fb_info_t *fbinfo) {
  return vga->vg_ops->get_fbinfo(vga, fbinfo);
}
static inline int vga_set_fbinfo(vga_device_t *vga, fb_info_t *fbinfo) {
  return vga->vg_ops->set_fbinfo(vga, fbinfo);
}

#endif /* !_DEV_VGA_H_ */
