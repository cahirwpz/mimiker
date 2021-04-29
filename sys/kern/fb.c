#include <sys/fb.h>
#include <sys/devfs.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/device.h>

fb_palette_t *fb_palette_create(size_t len) {
  fb_palette_t *palette = kmalloc(M_DEV, sizeof(fb_palette_t), M_ZERO);

  palette->len = len;
  palette->red = kmalloc(M_DEV, sizeof(uint8_t) * len, M_ZERO);
  palette->green = kmalloc(M_DEV, sizeof(uint8_t) * len, M_ZERO);
  palette->blue = kmalloc(M_DEV, sizeof(uint8_t) * len, M_ZERO);

  return palette;
}

void fb_palette_destroy(fb_palette_t *palette) {
  kfree(M_DEV, palette->red);
  kfree(M_DEV, palette->green);
  kfree(M_DEV, palette->blue);
  kfree(M_DEV, palette);
}