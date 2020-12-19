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
#include <sys/pmap.h>
#include <sys/kmem.h>

#define RTL8139_VENDOR_ID 0x10ec
#define RTL8139_DEVICE_ID 0x8139

typedef struct rtl8139_state {
  resource_t *regs;
  resource_t *irq_res;
  vaddr_t rx_buf;
  paddr_t paddr;
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

static intr_filter_t rtl8139_intr(void *data) {
  rtl8139_state_t *state = data;
  // TODO: add enqueue routines
  bus_write_2(state->regs, 0x3E, 0x1);
  return IF_FILTERED;
}

// TODO: replace all magic constants
int rtl8139_attach(device_t *dev) {
  rtl8139_state_t *state = dev->state;

  pci_enable_busmaster(dev);
  state->regs = device_take_memory(dev, 1, RF_ACTIVE);

  /*
   * XXX: there is no irq routing in mimiker
   *      so in this case we need to add irq manually
   */
  device_add_irq(dev, 0, 10);
  state->irq_res = device_take_irq(dev, 0, RF_ACTIVE);
  if (!state->regs || !state->irq_res) {
    klog("rtl8139 failed to init resources");
    return ENXIO;
  }
  bus_intr_setup(dev, state->irq_res, rtl8139_intr, NULL, state,
                 "RTL8139");

  // TODO: mark this memory as PMAP_NOCACHE
  // TODO: assure contig memory area
  state->rx_buf = (vaddr_t)kmem_alloc(2 * PAGESIZE, M_ZERO);
  if (!pmap_kextract(state->rx_buf, &state->paddr)) {
    klog("failed to find paddr");
    return ENXIO;
  }
  // TODO: introduce ring buffer

  if (rtl_reset(state)) {
    klog("RTL8139: Failed to reset device!");
    return ENXIO;
  }

  bus_write_4(state->regs, 0x30, state->paddr);
  bus_write_4(state->regs, 0x44, 0xf | (1 << 7));
  bus_write_1(state->regs, 0x37, 0x0C);
  // IMR - mask rx enable
  bus_write_2(state->regs, 0x3C, 0x1);

  return 0;
}

static driver_t rtl8139_driver = {
  .desc = "RTL8139 driver",
  .size = sizeof(rtl8139_state_t),
  .probe = rtl8139_probe,
  .attach = rtl8139_attach,
};

DEVCLASS_ENTRY(pci, rtl8139_driver);
