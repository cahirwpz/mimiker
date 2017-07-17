#include <complex.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#define WIDTH 640
#define HEIGHT 480

#define STR(x) #x

uint8_t image[WIDTH * HEIGHT];
uint8_t palette[256 * 3];

void prepare_videomode(void) {
  FILE *videomode_file = fopen("/dev/vga/videomode", "r+b");
  unsigned int width, height, bpp;
  fscanf(videomode_file, "%d %d %d", &width, &height, &bpp);
  printf("Current resolution: %dx%d, %d BPP\n", width, height, bpp);
  /* Write new configuration. */
  int n = fprintf(videomode_file, "640 480 8");
  assert(n > 0);
  fclose(videomode_file);
}

void prepare_palette(void) {
  for (unsigned int i = 0; i < 256; i++) {
    palette[i * 3 + 0] = i;
    palette[i * 3 + 1] = i * i / 255;
    palette[i * 3 + 2] = i * i / 255;
  }
  int palette_handle = open("/dev/vga/palette", O_WRONLY, 0);
  assert(palette_handle > 0);
  write(palette_handle, palette, 256 * 3);
  close(palette_handle);
}

void display_image(void) {
  int fb_handle = open("/dev/vga/fb", O_WRONLY, 0);
  assert(fb_handle > 0);
  write(fb_handle, image, WIDTH * HEIGHT);
  close(fb_handle);
}

int f(float re, float im) {
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

  prepare_videomode();
  prepare_palette();

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

      image[y * WIDTH + x] = f(re, im);
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

  display_image();

  return 0;
}
