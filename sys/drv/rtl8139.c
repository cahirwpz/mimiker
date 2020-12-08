/* RTL8139 driver */
#include <sys/mimiker.h>
#include <sys/pci.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/devfs.h>
#include <sys/devclass.h>

#define RTL8139_VENDOR_ID 0x10ec
#define RTL8139_DEVICE_ID 0x8139

typedef struct rtl8139_state {
  resource_t *regs;
  intr_handler_t *intr_handler;
} rtl8139_state_t;

static int rtl8139_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);
  return pci_device_match(pcid, RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
}

static int rtl8139_attach(device_t *dev) {
  return 0;
}

static driver_t rtl8139_driver = {
  .desc = "RTL8139 driver",
  .size = sizeof(rtl8139_state_t),
  .probe = rtl8139_probe,
  .attach = rtl8139_attach,
};

DEVCLASS_ENTRY(pci, rtl8139_driver);
