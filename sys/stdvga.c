#include <pci.h>
#include <vga.h>
#include <stdc.h>

#define VGA_PALETTE_SIZE 256 * 3

typedef struct stdvga_device {
  pci_device_t *pci_device;
  resource_t mem;
  resource_t io;

  unsigned int width;
  unsigned int height;

  uint8_t palette_buffer[VGA_PALETTE_SIZE];

  vga_device_t vga;
} stdvga_device_t;

#define STDVGA_FROM_VGA(vga)                                                   \
  (stdvga_device_t *)container_of(vga, stdvga_device_t, vga)

/* Detailed information about VGA registers is available at
   http://www.osdever.net/FreeVGA/vga/vga.htm */

#define VGA_PALETTE_ADDR 0x3C8
#define VGA_PALETTE_DATA 0x3C9
#define VGA_AR_ADDR 0x3C0
#define VGA_AR_PAS 0x20 /* Palette Address Source bit */

/* Offsets for accessing ioports via PCI BAR1 (MMIO) */
#define VGA_MMIO_OFFSET (-0x3c0 + 0x400)
#define VBE_MMIO_OFFSET (0x500)

/* Bochs VBE. Simplifies VGA graphics mode configuration a great deal. Some
   documentation is available at http://wiki.osdev.org/Bochs_VBE_Extensions */
#define VBE_DISPI_INDEX_XRES 1
#define VBE_DISPI_INDEX_YRES 2
#define VBE_DISPI_INDEX_BPP 3
#define VBE_DISPI_INDEX_ENABLE 4
#define VBE_DISPI_ENABLED 0x01 /* VBE Enabled bit */

/* The general overview of the QEMU std vga device is available at
   https://github.com/qemu/qemu/blob/master/docs/specs/standard-vga.txt */
#define VGA_QEMU_STDVGA_VENDOR_ID 0x1234
#define VGA_QEMU_STDVGA_DEVICE_ID 0x1111

static void stdvga_io_write(stdvga_device_t *vga, uint16_t reg, uint8_t value) {
  bus_space_write_1(&vga->io, reg + VGA_MMIO_OFFSET, value);
}
static uint8_t stdvga_io_read(stdvga_device_t *vga, uint16_t reg)
  __attribute__((unused));
static uint8_t stdvga_io_read(stdvga_device_t *vga, uint16_t reg) {
  return bus_space_read_1(&vga->io, reg + VGA_MMIO_OFFSET);
}
static void stdvga_vbe_write(stdvga_device_t *vga, uint16_t reg,
                             uint16_t value) {
  /* <<1 shift enables access to 16-bit registers. */
  bus_space_write_2(&vga->io, (reg << 1) + VBE_MMIO_OFFSET, value);
}
static uint16_t stdvga_vbe_read(stdvga_device_t *vga, uint16_t reg) {
  return bus_space_read_2(&vga->io, (reg << 1) + VBE_MMIO_OFFSET);
}

static void stdvga_palette_write_single(stdvga_device_t *vga, uint8_t offset,
                                        uint8_t r, uint8_t g, uint8_t b) {
  stdvga_io_write(vga, VGA_PALETTE_ADDR, offset);
  stdvga_io_write(vga, VGA_PALETTE_DATA, r >> 2);
  stdvga_io_write(vga, VGA_PALETTE_DATA, g >> 2);
  stdvga_io_write(vga, VGA_PALETTE_DATA, b >> 2);
}

static void stdvga_palette_write_buffer(stdvga_device_t *stdvga,
                                        const uint8_t buf[VGA_PALETTE_SIZE]) {
  for (int i = 0; i < VGA_PALETTE_SIZE / 3; i++)
    stdvga_palette_write_single(stdvga, i, buf[3 * i + 0], buf[3 * i + 1],
                                buf[3 * i + 2]);
}

static int stdvga_palette_write(vga_device_t *vga, uio_t *uio) {
  stdvga_device_t *stdvga = STDVGA_FROM_VGA(vga);
  int error = uiomove_frombuf(stdvga->palette_buffer, VGA_PALETTE_SIZE, uio);
  if (error)
    return error;
  /* TODO: Only update the modified area. */
  stdvga_palette_write_buffer(stdvga, stdvga->palette_buffer);
  return 0;
}

static void stdvga_fb_write_buffer(stdvga_device_t *vga,
                                   const uint8_t buf[320 * 200]) {
  bus_space_write_region_1(&vga->mem, 0, buf, 320 * 240);
}

static int stdvga_fb_write(vga_device_t *vga, uio_t *uio) {
  stdvga_device_t *stdvga = STDVGA_FROM_VGA(vga);
  /* TODO: I'd love to pass the uio to the bus directly, instead of using a
     buffer to write out bytes one by one... */
  static uint8_t buf[320 * 200];
  int error = uiomove_frombuf(buf, 320 * 200, uio);
  if (error)
    return error;
  stdvga_fb_write_buffer(stdvga, buf);
  return 0;
}

/* For now, this is allocated statically, because I don't want to create a
   memory pool just for a single struct! However, that means we are limited to a
   single device! */
static stdvga_device_t the_stdvga;

int stdvga_pci_attach(pci_device_t *pci) {
  if (pci->vendor_id != VGA_QEMU_STDVGA_VENDOR_ID ||
      pci->device_id != VGA_QEMU_STDVGA_DEVICE_ID)
    return 0;

  /* TODO: kmalloc */
  stdvga_device_t *stdvga = &the_stdvga;

  uint32_t status_command = pci_read_config(pci, 1, 4);
  uint32_t command = status_command & 0x0000ffff;
  uint32_t status = status_command & 0xffff0000;
  command |= 0x0003; /* Memory Space enable */
  status_command = status | command;
  pci_write_config(pci, 1 * 4, 4, status_command);

  assert(pci->bar[0].r_flags | RF_PREFETCHABLE);
  stdvga->mem = pci->bar[0];
  stdvga->io = pci->bar[1];

  /* Currently resolution is not configurable. */
  stdvga->width = 320;
  stdvga->height = 200;

  stdvga->vga = (vga_device_t){
    .palette_write = stdvga_palette_write, .fb_write = stdvga_fb_write,
  };

  /* Apply resolution */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_XRES, stdvga->width);
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_YRES, stdvga->height);

  /* Enable palette access */
  stdvga_io_write(stdvga, VGA_AR_ADDR, VGA_AR_PAS);

  /* Set 8 BPP */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_BPP, 8);

  /* Enable VBE. */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_ENABLE,
                   stdvga_vbe_read(stdvga, VBE_DISPI_INDEX_ENABLE) |
                     VBE_DISPI_ENABLED);

  /* Install /dev/vga interace. */
  dev_vga_install(&stdvga->vga);

  return 1;
}
