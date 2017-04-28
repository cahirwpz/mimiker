#ifndef _SYS_VGA_H_
#define _SYS_VGA_H_

#include <vnode.h>
#include <pci.h>
#include <uio.h>

typedef struct vga_control {
  uint32_t pci_memory;
  uint32_t pci_io;
  unsigned int width;
  unsigned int height;
} vga_control_t;

/* Initializes a vga_control_t structure using data from a particular
   pci_device. */
int vga_control_pci_init(pci_device_t *pci, vga_control_t *vga);

/* Read/write access to Graphic Registers */
uint8_t vga_gr_read(vga_control_t *vga, uint8_t gr);
void vga_gr_write(vga_control_t *vga, uint8_t gr, uint8_t val);

/* Read/write access to Sequencer Registers */
uint8_t vga_sr_read(vga_control_t *vga, uint8_t sr);
void vga_sr_write(vga_control_t *vga, uint8_t sr, uint8_t val);

/* Read/write access to Control Registers */
uint8_t vga_cr_read(vga_control_t *vga, uint8_t cr);
void vga_cr_write(vga_control_t *vga, uint8_t cr, uint8_t val);

/* Enables/disables palette write access. When enabled, display is not redrawn.
 */
void vga_set_pallete_addr_source(vga_control_t *vga, uint8_t v);

/* Set a single color in palette */
void vga_palette_write_single(vga_control_t *vga, uint8_t offset, uint8_t r,
                              uint8_t g, uint8_t b);

/* Write to vga palette. buf must be of 3*256 size. */
void vga_palette_write(vga_control_t *vga, const uint8_t buf[3 * 256]);

/* Write to vga frame buffer. buf must be of 320 * 200 size. */
void vga_fb_write_buffer(vga_control_t *vga, const uint8_t buf[320 * 200])
  __attribute__((hot));

/* uio write to frame buffer. */
int vga_fb_write(vga_control_t *vga, uio_t *uio) __attribute__((hot));

/* This function only supports resolutions up to 255, but this will do for
   now. */
void vga_set_resolution(vga_control_t *vga, int w, int h);

/* Initializes a vga device. */
void vga_init(vga_control_t *vga);

#endif /* !_SYS_VGA_H_ */
