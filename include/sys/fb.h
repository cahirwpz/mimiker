#ifndef _SYS_FB_H_
#define _SYS_FB_H_

#include <sys/ioccom.h>
#include <sys/types.h>

#define FB_IOC_MAGIC 'F'
#define FBIOCGET_FBINFO _IOR(FB_IOC_MAGIC, 1, struct fb_info)
#define FBIOCSET_FBINFO _IOW(FB_IOC_MAGIC, 1, struct fb_info)
#define FBIOCSET_PALETTE _IOW(FB_IOC_MAGIC, 2, struct fb_palette)

#define FBIOGET_VSCREENINFO _IOR(FB_IOC_MAGIC, 3, struct fb_var_screeninfo)
#define FBIOPUT_VSCREENINFO _IOW(FB_IOC_MAGIC, 3, struct fb_var_screeninfo)

struct fb_var_screeninfo {
  uint32_t xres, yres;
  uint32_t bits_per_pixel;
};

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
