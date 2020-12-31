/* RTL8139 driver */
#include <sys/mimiker.h>
#include <sys/pci.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/devfs.h>
#include <sys/devclass.h>
#include <sys/vnode.h>
#include <sys/pmap.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <dev/rtl8139_reg.h>

#define RTL8139_VENDOR_ID 0x10ec
#define RTL8139_DEVICE_ID 0x8139

typedef TAILQ_HEAD(skb_list, sk_buff) skb_list_t;

struct sk_buff {
  TAILQ_ENTRY(sk_buff) queue;
  size_t length;
  void *data;
};

typedef struct rtl8139_state {
  resource_t *regs;
  resource_t *irq_res;
  vaddr_t rx_buf;
  size_t rx_offset;
  paddr_t paddr;
  skb_list_t rx_queue;
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

static void do_receive(rtl8139_state_t *state) {
  /* the device insert their own header which contains size and status of packet
   */
  uint16_t size = *(uint16_t *)(state->rx_buf + state->rx_offset + 2);

  struct sk_buff *skb = kmalloc(M_DEV, sizeof(struct sk_buff), M_ZERO);
  skb->length = size;
  skb->data = kmalloc(M_DEV, size, M_ZERO);
  memcpy(skb->data, (void *)(state->rx_buf + state->rx_offset + 4), size);
  /* TODO: handle buffer overflow */
  /* Enqueue */
  /* TODO: WITH_MTX_LOCK { */
  TAILQ_INSERT_TAIL(&state->rx_queue, skb, queue);

  state->rx_offset += size + /* rtl8139 header */ 4;
  bus_write_2(state->regs, RL_ISR, RL_ISR_RX_OK);
}

static void do_transceive(rtl8139_state_t *state) {
  /* TODO */
}

static intr_filter_t rtl8139_intr(void *data) {
  rtl8139_state_t *state = data;
  uint16_t status = bus_read_2(state->regs, RL_ISR);

  if (status & RL_ISR_RX_OK)
    do_receive(state);
  else if (status & RL_ISR_TX_OK)
    do_transceive(state);

  return IF_FILTERED;
}

static int set_mem_contig_address(rtl8139_state_t *state) {
  paddr_t page2; /* address of second allocated page */

  /* set and check if any address is assigned */
  if (!pmap_kextract(state->rx_buf, &state->paddr))
    return -1;

  /* check if memory is contiguous */
  if (!pmap_kextract(state->rx_buf + PAGESIZE, &page2))
    return -1;

  if (state->paddr + PAGESIZE != page2)
    return -1;

  return 0;
}

static void skb_free(struct sk_buff *skb) {
  if (skb->length)
    kfree(M_DEV, skb->data);
  kfree(M_DEV, skb);
}

static int rtl8139_read(vnode_t *v, uio_t *uio, int ioflag) {
  rtl8139_state_t *state = devfs_node_data(v);
  /* TODO: WITH_MTX_LOCK { */
  uio->uio_offset = 0;
  struct sk_buff *skb = TAILQ_FIRST(&state->rx_queue);
  if (skb) {
    int res = uiomove_frombuf(skb->data, skb->length, uio);
    TAILQ_REMOVE(&state->rx_queue, skb, queue);
    skb_free(skb);
    return res;
  } else
    return uiomove_frombuf(NULL, 0, uio);
}

static vnodeops_t rtl8139_vnodeops = {.v_read = rtl8139_read};

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
  bus_intr_setup(dev, state->irq_res, rtl8139_intr, NULL, state, "RTL8139");

  TAILQ_INIT(&state->rx_queue);
  state->rx_offset = 0;
  state->rx_buf = (vaddr_t)kmem_alloc(2 * PAGESIZE, M_ZERO);
  // TODO: mark this memory as PMAP_NOCACHE
  if (set_mem_contig_address(state)) {
    klog("failed to alloc contig memory");
    return ENXIO;
  }

  if (rtl_reset(state)) {
    klog("RTL8139: Failed to reset device!");
    return ENXIO;
  }

  /* set-up address for dma */
  bus_write_4(state->regs, RL_RXADDR, state->paddr);
  /* we want to receive all packets - promiscious mode */
  /* 0xf - accept broadcast, multicast, physical match, all other packets
   * (promisc) */
  /* 0x80 - wrap bit */
  bus_write_4(state->regs, RL_RXCFG, 0x8f);
  bus_write_1(state->regs, RL_COMMAND, RL_CMD_RX_ENB | RL_CMD_TX_ENB);
  // IMR - mask rx enable
  bus_write_2(state->regs, RL_IMR, RL_ISR_RX_OK);

  devfs_makedev(NULL, "rtl8139", &rtl8139_vnodeops, state, NULL);
  return 0;
}

static driver_t rtl8139_driver = {
  .desc = "RTL8139 driver",
  .size = sizeof(rtl8139_state_t),
  .probe = rtl8139_probe,
  .attach = rtl8139_attach,
};

DEVCLASS_ENTRY(pci, rtl8139_driver);
