#include <ktest.h>
#include <stdc.h>
#include <vga.h>
#include <errno.h>
#include <mount.h>
#include <vnode.h>
#include <vm_map.h>

static void hue(uint8_t q, uint8_t *r, uint8_t *g, uint8_t *b) {
#define CH(x, y, z)                                                            \
  {                                                                            \
    x = r;                                                                     \
    y = g;                                                                     \
    x = b;                                                                     \
  }
  uint8_t x_, *x = &x_;
  uint8_t s = q / 42;
  uint8_t t = q % 42;
  /* Clip far end. Red stripe will be a few pixels too wide, but nobody will
     notice. */
  if (s == 6) {
    s = 5;
    t = 41;
  }
  assert(0 <= s && s <= 5);
  uint8_t *max = (uint8_t *[]){r, g, g, b, b, r}[s];
  uint8_t *min = (uint8_t *[]){b, b, r, r, g, g}[s];
  uint8_t *rise = (uint8_t *[]){g, x, b, x, r, x}[s];
  uint8_t *fall = (uint8_t *[]){x, r, x, g, x, b}[s];
  *max = 255;
  *min = 0;
  *rise = t * 6;
  *fall = (42 - t) * 6;
}

static uint8_t palette_buffer[256 * 3];
static uint8_t frame_buffer[320 * 200];

static int test_vga() {
  uio_t uio;
  iovec_t iov;
  vnode_t *dev_vga_fb, *dev_vga_palette;
  int error, frames = 1000;

  error = vfs_lookup("/dev/vga/fb", &dev_vga_fb);
  assert(error == 0);
  error = vfs_lookup("/dev/vga/palette", &dev_vga_palette);
  assert(error == 0);

  /* Prepare palette */
  for (int i = 0; i < 256; i++) {
    hue(i, palette_buffer + 3 * i + 0, palette_buffer + 3 * i + 1,
        palette_buffer + 3 * i + 2);
  }

  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_kernel_vm_map();
  uio.uio_offset = 0;
  uio.uio_resid = 256 * 3;
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  iov.iov_base = palette_buffer;
  iov.iov_len = 256 * 3;

  error = VOP_WRITE(dev_vga_palette, &uio);
  assert(error == 0);
  assert(uio.uio_resid == 0);

  /* Framebuffer */

  int off = 0;
  while (frames--) {
    for (int i = 0; i < 320 * 200; i++)
      frame_buffer[i] = (i % 64) ? ((i / 320 + off) % 256) : (0);

    uio.uio_op = UIO_WRITE;
    uio.uio_vmspace = get_kernel_vm_map();
    uio.uio_offset = 0;
    uio.uio_resid = 320 * 200;
    uio.uio_iovcnt = 1;
    uio.uio_iov = &iov;
    iov.iov_base = frame_buffer;
    iov.iov_len = 320 * 200;

    error = VOP_WRITE(dev_vga_fb, &uio);
    assert(error == 0);
    assert(uio.uio_resid == 0);

    off++;
  }

  kprintf("Displayed 1000 frames.\n");

  return KTEST_SUCCESS;
}

KTEST_ADD(vga, test_vga, KTEST_FLAG_NORETURN);
