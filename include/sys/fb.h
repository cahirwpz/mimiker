#ifndef _SYS_FB_H_
#define _SYS_FB_H_

#include <sys/ioccom.h>
#include <sys/types.h>

#define FB_IOC_MAGIC 'F'
#define FBIOCGET_FBINFO _IOR(FB_IOC_MAGIC, 0x01, struct fb_info)
#define FBIOCSET_FBINFO _IOW(FB_IOC_MAGIC, 0x01, struct fb_info)
#define FBIOCSET_PALETTE _IOW(FB_IOC_MAGIC, 0x02, struct fb_palette)

struct fb_color {
  uint8_t r, g, b;
};

struct fb_palette {
  uint32_t len;
  struct fb_color *colors;
};

struct fb_info {
  uint32_t width;
  uint32_t height;
  uint32_t bpp; /* bits per pixel */
};

#endif /* !_SYS_FB_H_ */
