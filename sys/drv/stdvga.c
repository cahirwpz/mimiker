/* Standard VGA driver */
#include <sys/klog.h>
#include <dev/pci.h>
#include <dev/vga.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/fb.h>
#include <sys/devclass.h>

#define VGA_PALETTE_SIZE 256

typedef struct stdvga_state {
  resource_t *mem;
  resource_t *io;

  fb_info_t fb_info;

  fb_palette_t *palette;
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

static void stdvga_palette_write_all(stdvga_state_t *stdvga) {
  for (size_t i = 0; i < stdvga->palette->len; i++)
    stdvga_palette_write_single(stdvga, i, stdvga->palette->red[i],
                                stdvga->palette->green[i],
                                stdvga->palette->blue[i]);
}

static int stdvga_palette_write(vga_device_t *vga, fb_palette_t *palette) {
  stdvga_state_t *stdvga = STDVGA_FROM_VGA(vga);
  size_t len = stdvga->palette->len;

  if (len != palette->len)
    return EINVAL;

  memcpy(stdvga->palette->red, palette->red, len);
  memcpy(stdvga->palette->green, palette->green, len);
  memcpy(stdvga->palette->blue, palette->blue, len);

  /* TODO: Only update the modified area. */
  stdvga_palette_write_all(stdvga);
  return 0;
}

static int stdvga_get_fbinfo(vga_device_t *vga, fb_info_t *fb_info) {
  stdvga_state_t *stdvga = STDVGA_FROM_VGA(vga);
  memcpy(fb_info, &stdvga->fb_info, sizeof(fb_info_t));
  return 0;
}

static int stdvga_set_fbinfo(vga_device_t *vga, fb_info_t *fb_info) {
  stdvga_state_t *stdvga = STDVGA_FROM_VGA(vga);

  /* Impose some reasonable resolution limit. As long as we have to use an
     fb_buffer, the limit is related to the size of memory pool used by the
     graphics driver. */
  if (fb_info->width > 640 || fb_info->height > 480)
    return EINVAL;

  if (fb_info->bpp != 8 && fb_info->bpp != 16 && fb_info->bpp != 24)
    return EINVAL;

  /* We keep the size of the potentially previously allocated fb_buffer */
  int previous_size = align(
    sizeof(uint8_t) * stdvga->fb_info.width * stdvga->fb_info.height, PAGESIZE);

  memcpy(&stdvga->fb_info, fb_info, sizeof(fb_info_t));

  /* Apply resolution */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_XRES, stdvga->fb_info.width);
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_YRES, stdvga->fb_info.height);

  /* Set BPP */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_BPP, stdvga->fb_info.bpp);

  int aligned_size = align(
    sizeof(uint8_t) * stdvga->fb_info.width * stdvga->fb_info.height, PAGESIZE);

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
  int error = uiomove_frombuf(
    stdvga->fb_buffer, stdvga->fb_info.width * stdvga->fb_info.height, uio);
  if (error)
    return error;
  bus_write_region_1(stdvga->mem, 0, stdvga->fb_buffer,
                     stdvga->fb_info.width * stdvga->fb_info.height);
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

static vga_ops_t stdvga_ops = {
  .palette_write = stdvga_palette_write,
  .fb_write = stdvga_fb_write,
  .get_fbinfo = stdvga_get_fbinfo,
  .set_fbinfo = stdvga_set_fbinfo,
};

static int stdvga_attach(device_t *dev) {
  stdvga_state_t *stdvga = dev->state;

  stdvga->mem = device_take_memory(dev, 0, RF_ACTIVE | RF_PREFETCHABLE);
  stdvga->io = device_take_memory(dev, 2, RF_ACTIVE);

  assert(stdvga->mem != NULL);
  assert(stdvga->io != NULL);

  stdvga->vga = (vga_device_t){
    .vg_usecnt = 0,
    .vg_ops = &stdvga_ops,
  };

  /* Prepare palette buffers */
  stdvga->palette = fb_palette_create(VGA_PALETTE_SIZE);

  /* Apply resolution */
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_XRES, stdvga->fb_info.width);
  stdvga_vbe_write(stdvga, VBE_DISPI_INDEX_YRES, stdvga->fb_info.height);

  /* Enable palette access */
  stdvga_io_write(stdvga, VGA_AR_ADDR, VGA_AR_PAS);

  /* Configure initial videomode. */
  fb_info_t def_fb_info = {.width = 320, .height = 200, .bpp = 8};
  stdvga_set_fbinfo(&stdvga->vga, &def_fb_info);

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
  .pass = SECOND_PASS,
  .probe = stdvga_probe,
  .attach = stdvga_attach,
};

DEVCLASS_ENTRY(pci, stdvga);
