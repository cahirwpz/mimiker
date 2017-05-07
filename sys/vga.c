#include <pci.h>
#include <stdc.h>

/* Normally this file an intialization procedure would not exits! This is barely
   a substitute for currently unimplemented device-driver matching mechanism. */

extern pci_bus_device_t gt_pci;
extern int stdvga_pci_attach(pci_device_t *pci);

/* Searches for and enables a vga device. */
void vga_init() {
  /* Look at all pci devices. */
  device_t *dev;
  TAILQ_FOREACH (dev, &gt_pci.dev.children, link) {
    /* Try initializing it as an stdvga device. */
    int attach = stdvga_pci_attach((pci_device_t *)dev);
    if (attach)
      return;
    /* If we had more drivers, this loop would proceed to the next one and try
       attaching it to the next device. */
  }
}
