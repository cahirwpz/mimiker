/* Standard VGA driver */
#include <sys/pci.h>
#include <sys/vga.h>
#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/devclass.h>

#define VGA_PALETTE_SIZE (256 * 3)

typedef struct stdvga_state {
  resource_t *mem;
  resource_t *io;

  unsigned int width;
  unsigned int height;
  unsigned int bpp;

  uint8_t *palette_buffer;
  uint8_t *fb_buffer; /* This buffer is only needed because we can't pass uio
                         directly to the bus. */
  vga_device_t vga;
} stdvga_state_t;

#define STDVGA_FROM_VGA(vga)                                                   \
  (stdvga_state_t *)container_of(vga, stdvga_state_t, vga)

/* Detailed information about VGA registers is available at
   http://www.osdever.net/FreeVGA/vga/vga.htm */

#define VGA_PALETTE_ADDR 0x3C8
#define VGA_PALETTE_DATA 0x3C9
#define VGA_AR_ADDR 0x3C0
#define VGA_AR_PAS 0x20 /* Palette Address Source bit */

/* Bochs VBE. Simplifies VGA graphics mode configuration a great deal. Some
   documentation is available at http://wiki.osdev.org/Bochs_VBE_Extensions */
#define VBE_DISPI_INDEX_XRES 0x01
#define VBE_DISPI_INDEX_YRES 0x02
#define VBE_DISPI_INDEX_BPP 0x03
#define VBE_DISPI_INDEX_ENABLE 0x04
#define VBE_DISPI_ENABLED 0x01 /* VBE Enabled bit */

/* Offsets for accessing ioports via PCI BAR1 (MMIO) */
#define VGA_MMIO_OFFSET (0x400 - 0x3c0)
#define VBE_MMIO_OFFSET 0x500

/* The general overview of the QEMU std vga device is available at
   https://github.com/qemu/qemu/blob/master/docs/specs/standard-vga.txt */
#define QEMU_STDVGA_VENDOR_ID 0x1234
#define QEMU_STDVGA_DEVICE_ID 0x1111

static void stdvga_io_write(stdvga_state_t *vga, uint16_t reg, uint8_t value) {
  bus_write_1(vga->io, reg + VGA_MMIO_OFFSET, value);
}
static uint8_t __unused stdvga_io_read(stdvga_state_t *vga, uint16_t reg) {
  return bus_read_1(vga->io, reg + VGA_MMIO_OFFSET);
}
static void stdvga_vbe_write(stdvga_state_t *vga, uint16_t reg,
                             uint16_t value) {
  /* <<1 shift enables access to 16-bit registers. */
  bus_write_2(vga->io, (reg << 1) + VBE_MMIO_OFFSET, value);
}
static uint16_t stdvga_vbe_read(stdvga_state_t *vga, uint16_t reg) {
  return bus_read_2(vga->io, (reg << 1) + VBE_MMIO_OFFSET);
}

static void stdvga_palette_write_single(stdvga_state_t *stdvga, uint8_t offset,
                                        uint8_t r, uint8_t g, uint8_t b) {
  stdvga_io_write(stdvga, VGA_PALETTE_ADDR, offset);
  stdvga_io_write(stdvga, VGA_PALETTE_DATA, r >> 2);
  stdvga_io_write(stdvga, VGA_PALETTE_DATA, g >> 2);
  stdvga_io_write(stdvga, VGA_PALETTE_DATA, b >> 2);
}

static void stdvga_palette_write_buffer(stdvga_state_t *stdvga,
                                        const uint8_t buf[VGA_PALETTE_SIZE]) {
  for (int i = 0; i < VGA_PALETTE_SIZE / 3; i++)
    stdvga_palette_write_single(stdvga, i, buf[3 * i + 0], buf[3 * i + 1],
                                buf[3 * i + 2]);
}

static int stdvga_palette_write(vga_device_t *vga, uio_t *uio) {
  stdvga_state_t *stdvga = STDVGA_FROM_VGA(vga);
  int error = uiomove_frombuf(stdvga->palette_buffer, VGA_PALETTE_SIZE, uio);
  if (error)
    return error;
  /* TODO: Only update the modified area. */
  stdvga_palette_write_buffer(stdvga, stdvga->palette_buffer);
  return 0;
}

static int stdvga_get_videomode(vga_device_t *vga, unsigned *xres,
                                unsigned *yres, unsigned *bpp) {
  stdvga_state_t *stdvga = STDVGA_FROM_VGA(vga);
  *xres = stdvga->width;
  *yres = stdvga->height;
  *bpp = stdvga->bpp;
  return 0;
}

static int stdvga_set_videomode(vga_device_t *vga, unsigned xres, unsigned yres,
                                unsigned bpp) {
  stdvga_state_t *stdvga = STDVGA_FROM_VGA(vga);

  /* Impose some reasonable resolution limit. As long as we have to use an
     fb_buffer, the limit is related to the size of memory pool used by the
     graphics driver. */
  if (xres > 640 || yres > 480)
    return EINVAL;

  if (bpp != 8 && bpp != 16 && bpp != 24)
    return EINVAL;

  /* We keep the size of the potentially previously allocated fb_buffer */
  int previous_size =
    align(sizeof(uint8_t) * stdvga->width * stdvga->height, PAGESIZE);

  stdvga->width = xres;
  stdvga->height = yres;
  stdvga->bpp = bpp;

  /* Apply resolution */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_XRES, stdvga->width);
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_YRES, stdvga->height);

  /* Set BPP */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_BPP, stdvga->bpp);

  int aligned_size =
    align(sizeof(uint8_t) * stdvga->width * stdvga->height, PAGESIZE);

  if (stdvga->fb_buffer)
    kmem_free(stdvga->fb_buffer, previous_size);

  stdvga->fb_buffer = kmem_alloc(aligned_size, M_ZERO);

  return 0;
}

static int stdvga_fb_write(vga_device_t *vga, uio_t *uio) {
  stdvga_state_t *stdvga = STDVGA_FROM_VGA(vga);
  /* TODO: Some day `bus_space_map` will be implemented. This will allow to map
   * RT_MEMORY resource into kernel virtual address space. BUS_SPACE_MAP_LINEAR
   * would be ideal for frambuffer memory, since we could access it directly. */
  int error =
    uiomove_frombuf(stdvga->fb_buffer, stdvga->width * stdvga->height, uio);
  if (error)
    return error;
  bus_write_region_1(stdvga->mem, 0, stdvga->fb_buffer,
                     stdvga->width * stdvga->height);
  return 0;
}

static int stdvga_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);

  if (!pci_device_match(pcid, QEMU_STDVGA_VENDOR_ID, QEMU_STDVGA_DEVICE_ID))
    return 0;

  if (!(pcid->bar[0].flags & RF_PREFETCHABLE))
    return 0;

  return 1;
}

static int stdvga_attach(device_t *dev) {
  stdvga_state_t *stdvga = dev->state;

  stdvga->mem =
    bus_alloc_resource_any(dev, RT_MEMORY, 0, RF_ACTIVE | RF_PREFETCHABLE);
  stdvga->io = bus_alloc_resource_any(dev, RT_MEMORY, 1, RF_ACTIVE);

  assert(stdvga->mem != NULL);
  assert(stdvga->io != NULL);

  stdvga->vga = (vga_device_t){
    .palette_write = stdvga_palette_write,
    .fb_write = stdvga_fb_write,
    .get_videomode = stdvga_get_videomode,
    .set_videomode = stdvga_set_videomode,
  };

  /* Prepare palette buffer */
  stdvga->palette_buffer =
    kmalloc(M_DEV, sizeof(uint8_t) * VGA_PALETTE_SIZE, M_ZERO);

  /* Apply resolution */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_XRES, stdvga->width);
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_YRES, stdvga->height);

  /* Enable palette access */
  stdvga_io_write(stdvga, VGA_AR_ADDR, VGA_AR_PAS);

  /* Configure initial videomode. */
  stdvga_set_videomode(&stdvga->vga, 320, 200, 8);

  /* Enable VBE. */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_ENABLE,
                   stdvga_vbe_read(stdvga, VBE_DISPI_INDEX_ENABLE) |
                     VBE_DISPI_ENABLED);

  /* Install /dev/vga interace. */
  dev_vga_install(&stdvga->vga);

  return 0;
}

static driver_t stdvga = {
  .desc = "Bochs VGA driver",
  .size = sizeof(stdvga_state_t),
  .probe = stdvga_probe,
  .attach = stdvga_attach,
};

DEVCLASS_ENTRY(pci, stdvga);
