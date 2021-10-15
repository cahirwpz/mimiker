#include <complex.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fb.h>
#include <sys/ioctl.h>

#define WIDTH 640
#define HEIGHT 480
#define PALETTE_LEN 256

#define STR(x) #x

static uint8_t image[WIDTH * HEIGHT];
static uint8_t palette_buff[PALETTE_LEN * 3];

static void prepare_videomode(int vgafd) {
  struct fb_info fb_info;

  ioctl(vgafd, FBIOCGET_FBINFO, &fb_info);
  printf("Current resolution: %dx%d, %d BPP\n", fb_info.width, fb_info.height,
         fb_info.bpp);

  /* Write new configuration. */
  fb_info.width = WIDTH;
  fb_info.height = HEIGHT;
  fb_info.bpp = 8;
  ioctl(vgafd, FBIOCSET_FBINFO, &fb_info);
}

static void prepare_palette(int vgafd) {
  struct fb_palette palette = {
    .len = PALETTE_LEN,
    .colors = (void *)palette_buff,
  };

  for (unsigned int i = 0; i < PALETTE_LEN; i++) {
    palette.colors[i].r = i;
    palette.colors[i].g = i * i / 255;
    palette.colors[i].b = i * i / 255;
  }

  ioctl(vgafd, FBIOCSET_PALETTE, &palette);
}

static void display_image(int vgafd) {
  write(vgafd, image, WIDTH * HEIGHT);
}

static int fun(float re, float im) {
  float re0 = re, im0 = im;
  unsigned int n = 0;
  for (n = 0; n < 50; n++) {
    float xt = re * re - im * im + re0;
    im = 2 * im * re + im0;
    re = xt;
    if (im * im + re * re > 50000.0f)
      break;
  };
  return (50 - n) * 250 / 50;
}

int main(void) {
  int vgafd = open("/dev/vga", O_RDWR, 0);
  if (vgafd < 0) {
    printf("can't open /dev/vga file\n");
    return 1;
  }

  prepare_videomode(vgafd);
  prepare_palette(vgafd);

  for (unsigned int y = 0; y < HEIGHT; y++) {
    for (unsigned int x = 0; x < WIDTH; x++) {
      float re = (x / (float)WIDTH) * 2.0f - 1.0f;
      float im = (y / (float)HEIGHT) * 2.0f - 1.0f;
      im *= -1 * (HEIGHT / (float)WIDTH);

      /* Correct center. */
      re -= 0.25f;

      /* Apply scale. */
      re *= 1.8f;
      im *= 1.8f;

      image[y * WIDTH + x] = fun(re, im);
    }
  }

  /* Draw color scale at the top of the screen. */
  for (unsigned int x = 0; x < WIDTH; x++) {
    int q = 256.0f * x / WIDTH;
    image[0 * WIDTH + x] = 0;
    image[1 * WIDTH + x] = q;
    image[2 * WIDTH + x] = q;
    image[3 * WIDTH + x] = q;
    image[4 * WIDTH + x] = q;
    image[5 * WIDTH + x] = q;
    image[6 * WIDTH + x] = 0;
  }

  display_image(vgafd);

  close(vgafd);

  return 0;
}
