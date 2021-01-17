/* RTL8139 driver */
#include <sys/mimiker.h>
#include <sys/pci.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/devfs.h>
#include <sys/devclass.h>
#include <dev/rtl8139_reg.h>

#define RTL8139_VENDOR_ID 0x10ec
#define RTL8139_DEVICE_ID 0x8139

typedef struct rtl8139_state {
  resource_t *regs;
} rtl8139_state_t;

static int rtl8139_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);
  return pci_device_match(pcid, RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
}

static int rtl_reset(rtl8139_state_t *state) {
  bus_write_1(state->regs, RL_8139_CFG1, RL_CFG1_PWRON);
  bus_write_1(state->regs, RL_COMMAND, RL_CMD_RESET);
  for (int i = 0; i < RL_TIMEOUT; i++) {
    if (!(bus_read_1(state->regs, RL_COMMAND) & RL_CMD_RESET))
      return 0;
  }

  return 1;
}

static int rtl8139_attach(device_t *dev) {
  rtl8139_state_t *state = dev->state;

  state->regs = device_take_memory(dev, 1, RF_ACTIVE);
  if (state->regs == NULL)
    return ENXIO;

  if (rtl_reset(state)) {
    klog("RTL8139: Failed to reset device!");
    return ENXIO;
  }

  return 0;
}

static driver_t rtl8139_driver = {
  .desc = "RTL8139 driver",
  .size = sizeof(rtl8139_state_t),
  .pass = SECOND_PASS,
  .probe = rtl8139_probe,
  .attach = rtl8139_attach,
};

DEVCLASS_ENTRY(pci, rtl8139_driver);
