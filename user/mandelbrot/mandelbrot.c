#include <complex.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#define WIDTH 320
#define HEIGHT 200

uint8_t image[WIDTH * HEIGHT];
uint8_t palette[256 * 3];

void prepare_palette() {
  for (unsigned int i = 0; i < 256; i++) {
    palette[i * 3 + 0] = i;
    palette[i * 3 + 1] = i * i / 255;
    palette[i * 3 + 2] = i * i / 255;
  }
  int palette_handle = open("/dev/vga/palette", O_WRONLY, 0);
  assert(palette_handle > 0);
  write(palette_handle, palette, 256 * 3);
}

void display_image() {
  int fb_handle = open("/dev/vga/fb", O_WRONLY, 0);
  assert(fb_handle > 0);
  write(fb_handle, image, WIDTH * HEIGHT);
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

int main() {

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
