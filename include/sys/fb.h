#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <sys/ioccom.h>
#include <sys/types.h>

#define FB_IOC_MAGIC 'F'
#define FBIOCGET_FBINFO _IOR(FB_IOC_MAGIC, 0x01, struct fb_info)
#define FBIOCSET_FBINFO _IOW(FB_IOC_MAGIC, 0x01, struct fb_info)

#define FBIOCSET_PALETTE _IOW(FB_IOC_MAGIC, 0x02, struct fb_palette)

#ifdef _KERNEL
typedef struct fb_palette fb_palette_t;
typedef struct fb_info fb_info_t;
#endif /* _KERNEL */

struct fb_palette {
  uint32_t len;

  struct {
    uint8_t r, g, b;
  } * colors;
};

struct fb_info {
  uint32_t width;
  uint32_t height;
  uint32_t bpp; /* bits per pixel */
};

#endif /* _FRAMEBUFFER_H */