#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#ifndef __KERNEL__
#include <sys/ioccom.h>
#include <sys/types.h>
#endif

#define FB_IOC_MAGIC 'F'
#define FBIOCGET_FBINFO _IOR(FB_IOC_MAGIC, 0x01, struct fb_info)
#define FBIOCSET_FBINFO _IOW(FB_IOC_MAGIC, 0x01, struct fb_info)

#define FBIOCSET_PALETTE _IOW(FB_IOC_MAGIC, 0x02, struct fb_palette)

#ifdef _KERNEL
typedef struct fb_palette fb_palette_t;
typedef struct fb_info fb_info_t;

/*
 * Allocate fb_palette_t for `len` colours.
 */
fb_palette_t *fb_palette_create(size_t len);

/*
 * Deallocate palette and its buffers.
 */
void fb_palette_destroy(fb_palette_t *palette);
#endif /* _KERNEL */

struct fb_palette {
  uint32_t len;
  uint8_t *red;
  uint8_t *green;
  uint8_t *blue;
};

struct fb_info {
  uint32_t width;
  uint32_t height;
  uint32_t bpp; /* bits per pixel */
};

#endif /* _FRAMEBUFFER_H */