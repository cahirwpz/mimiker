#include <vga.h>
#include <mips/pci.h>
#include <stdc.h>
#include <errno.h>

#define VGA_PCI_IO(vga, x)                                                     \
  (volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(vga->pci_io) + (x))

#define VGA_GR_ADDR(vga) VGA_PCI_IO(vga, 0x3CE)
#define VGA_GR_DATA(vga) VGA_PCI_IO(vga, 0x3CF)
#define VGA_CR_ADDR(vga) VGA_PCI_IO(vga, 0x3B4)
#define VGA_CR_DATA(vga) VGA_PCI_IO(vga, 0x3B5)
#define VGA_SR_ADDR(vga) VGA_PCI_IO(vga, 0x3C4)
#define VGA_SR_DATA(vga) VGA_PCI_IO(vga, 0x3C5)
#define VGA_PALETTE_ADDR(vga) VGA_PCI_IO(vga, 0x3C8)
#define VGA_PALETTE_DATA(vga) VGA_PCI_IO(vga, 0x3C9)
#define VGA_AR_ADDR(vga) VGA_PCI_IO(vga, 0x3C0)
#define VGA_AR_DATA_WRITE(vga) VGA_PCI_IO(vga, 0x3C0)
#define VGA_AR_DATA_READ(vga) VGA_PCI_IO(vga, 0x3C1)
#define VGA_REG_EXT_MOR_READ VGA_PCI_IO(vga, 0x3CC)
#define VGA_REG_EXT_MOR_WRITE VGA_PCI_IO(vga, 0x3C2)

#define VGA_FB(vga, n)                                                         \
  (volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(vga->pci_memory) + (n))

#define VGA_CIRRUS_LOGIC_VENDOR_ID 0x1013
#define VGA_CIRRUS_LOGIC_DEVICE_ID_GD5446 0x00b8

/* It feels like they picked these numbers at random. */
#define QEMU_STD_VGA_VENDOR_ID 0x1234
#define QEMU_STD_VGA_DEVICE_ID 0x1111

int vga_control_pci_init(pci_device_t *pci, vga_control_t *vga) {
  /* Access the PCI command register of the device. Enable memory space. */

  bzero(vga, sizeof(vga_control_t));

  if (pci->vendor_id == VGA_CIRRUS_LOGIC_VENDOR_ID &&
      pci->device_id == VGA_CIRRUS_LOGIC_DEVICE_ID_GD5446)
    goto match;
  if (pci->vendor_id == QEMU_STD_VGA_VENDOR_ID &&
      pci->device_id == QEMU_STD_VGA_DEVICE_ID)
    goto match;

  return ENOTSUP;

match:
  PCI0_CFG_ADDR_R =
    PCI0_CFG_ENABLE | PCI0_CFG_REG(pci->addr.device, pci->addr.function, 1);
  uint32_t status_command = PCI0_CFG_DATA_R;
  uint32_t command = status_command & 0x0000ffff;
  uint32_t status = status_command & 0xffff0000;
  command |= 0x0003; /* Memory Space enable */
  status_command = status | command;
  PCI0_CFG_DATA_R = status_command;

  /* Search bars for VGA memory */
  for (int i = 0; i < pci->nbars; i++) {
    pci_bar_t *bar = pci->bar + i;
    pm_addr_t addr = bar->addr;

    if (addr & PCI_BAR_PREFETCHABLE) {
      /* This looks like VGA memory area */
      if (vga->pci_memory == 0)
        vga->pci_memory = addr & ~PCI_BAR_MEMORY_MASK;
    }
  }

  if (vga->pci_memory == 0) {
    log("Failed to locate VGA memory address");
    return ENOTSUP;
  }

  vga->pci_io = 0x18000000;

  return 0;
}

uint8_t vga_gr_read(vga_control_t *vga, uint8_t gr) {
  *VGA_GR_ADDR(vga) = gr;
  return *VGA_GR_DATA(vga);
}
void vga_gr_write(vga_control_t *vga, uint8_t gr, uint8_t val) {
  *VGA_GR_ADDR(vga) = gr;
  *VGA_GR_DATA(vga) = val;
}
uint8_t vga_cr_read(vga_control_t *vga, uint8_t cr) {
  *VGA_CR_ADDR(vga) = cr;
  return *VGA_CR_DATA(vga);
}
void vga_cr_write(vga_control_t *vga, uint8_t cr, uint8_t val) {
  *VGA_CR_ADDR(vga) = cr;
  *VGA_CR_DATA(vga) = val;
}
uint8_t vga_sr_read(vga_control_t *vga, uint8_t sr) {
  *VGA_SR_ADDR(vga) = sr;
  return *VGA_SR_DATA(vga);
}
void vga_sr_write(vga_control_t *vga, uint8_t sr, uint8_t val) {
  *VGA_SR_ADDR(vga) = sr;
  *VGA_SR_DATA(vga) = val;
}

void vga_set_pallete_addr_source(vga_control_t *vga, uint8_t v) {
  *VGA_AR_ADDR(vga) |= v << 5;
}

void vga_palette_write_single(vga_control_t *vga, uint8_t offset, uint8_t r,
                              uint8_t g, uint8_t b) {
  *VGA_PALETTE_ADDR(vga) = offset;
  *VGA_PALETTE_DATA(vga) = r >> 2;
  *VGA_PALETTE_DATA(vga) = g >> 2;
  *VGA_PALETTE_DATA(vga) = b >> 2;
}

void vga_palette_write(vga_control_t *vga, const uint8_t buf[3 * 256]) {
  for (int i = 0; i < 256; i++)
    vga_palette_write_single(vga, i, buf[3 * i + 0], buf[3 * i + 1],
                             buf[3 * i + 2]);
}

void vga_set_resolution(vga_control_t *vga, int w, int h) {
  uint8_t wq = w / 8 - 1;
  *VGA_CR_ADDR(vga) = 0x01;
  *VGA_CR_DATA(vga) = wq;
  *VGA_CR_ADDR(vga) = 0x12;
  *VGA_CR_DATA(vga) = h - 1;
}

void vga_fb_write_buffer(vga_control_t *vga, const uint8_t buf[320 * 200]) {
  for (int i = 0; i < 320 * 200; i++) {
    volatile uint8_t *q = VGA_FB(vga, i);
    *q = buf[i];
  }
}

int vga_fb_write(vga_control_t *vga, uio_t *uio) {
  return uiomove((char *)(VGA_FB(vga, 0)), 320 * 200, uio);
}

void vga_init(vga_control_t *vga) {
  /* Set RAM_ENABLE */
  *VGA_REG_EXT_MOR_WRITE = 0x2;

  // Set memory map select to 0
  uint8_t memory_map_select = vga_gr_read(vga, 0x06) >> 2 & 0x3;
  memory_map_select = 0;
  vga_gr_write(vga, 0x06, memory_map_select << 2);

  // Selected memory range A0000h-BFFFFh. Probably irrelevant, as we use PCI LFB

  // Set chain 4 enable (? probably unnecessary for PCI LFB writing
  vga_sr_write(vga, 0x04, vga_sr_read(vga, 0x04) | 0x08);

  // Enable 256-color palette
  vga_gr_write(vga, 0x05, vga_gr_read(vga, 0x05) | 0x40);

  // Set both Map Display Addresses to 1
  vga_cr_write(vga, 0x17, vga_cr_read(vga, 0x17) | 0x03);

  // Switch from text mode to graphics mode
  vga_gr_write(vga, 0x06, vga_gr_read(vga, 0x06) | 0x01);

  // Apply resolution
  vga_set_resolution(vga, 320, 200);

  // Set 8 bits per pixel
  /* This is not standard VGA, but a Cirrus extension. */
  vga_sr_write(vga, 0x07, 0x01);

  // Set line offset register
  vga_cr_write(vga, 0x13, 320 / 8);
  // Set line compare register
  vga_cr_write(vga, 0x18, 200);

  // Enable palette access
  vga_set_pallete_addr_source(vga, 1);
}
