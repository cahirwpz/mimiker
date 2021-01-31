#define KL_LOG KL_USB
#include <sys/bus.h>
#include <sys/pci.h>
#include <sys/devclass.h>
#include <sys/libkern.h>
#include <sys/klog.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/vm_physmem.h>
#include <sys/spinlock.h>
#include <sys/time.h>
#include <dev/usb.h>
#include <dev/usbhc.h>
#include <dev/uhci.h>
#include <dev/uhcireg.h>

#define UHCI_NQUEUES 5
#define UHCI_BUF_SIZE 1024
#define UHCI_BUF_NUM (PAGESIZE / UHCI_BUF_SIZE)
#define UHCI_BUF_FREE 1

typedef struct uhci_state {
  uhci_qh_t *queues[UHCI_NQUEUES]; /* schedule queues */
  void *pool;                      /* one page for physical buffers */
  spin_t lock;                     /* UHCI pool guard */
  uint32_t *frames;                /* UHCI frame list */
  resource_t *regs;                /* host controller registers */
  resource_t *irq;                 /* host controller interrupt */
  uint8_t nports;                  /* number of roothub ports */
} uhci_state_t;

/*
 * The following definitions assume an implicit `uhci` argument.
 */

/*
 * Read byte/word/double word from specified UHCI port.
 */
#define inb(addr) bus_read_1(uhci->regs, (addr))
#define inw(addr) bus_read_2(uhci->regs, (addr))
#define ind(addr) bus_read_4(uhci->regs, (addr))

/*
 * Write byte/word/double word to specified UHCI port.
 */
#define outb(addr, val) bus_write_1(uhci->regs, (addr), (val))
#define outw(addr, val) bus_write_2(uhci->regs, (addr), (val))
#define outd(addr, val) bus_write_4(uhci->regs, (addr), (val))

/*
 * Set specified bit in a given UHCI port.
 */
#define setb(p, b) outb((p), inb(p) | (b))
#define setw(p, b) outw((p), inw(p) | (b))
#define setd(p, b) outd((p), ind(p) | (b))

/*
 * Clear specified bit in a given UHCI port.
 */
#define clrb(p, b) outb((p), inb(p) & ~(b))
#define clrw(p, b) outw((p), inw(p) & ~(b))
#define clrd(p, b) outd((p), ind(p) & ~(b))

/*
 * Clear specified bit in a given UHCI write clear port.
 */
#define wclrb(p, b) setb((p), b)
#define wclrw(p, b) setw((p), b)
#define wclrd(p, b) setd((p), b)

/*
 * Check if a specified bit is set in a given UHCI port.
 */
#define chkb(p, b) (inb(p) & (b))
#define chkw(p, b) (inw(p) & (b))
#define chkd(p, b) (ind(p) & (b))

static int uhci_print_usb_release(device_t *dev) {
  uint8_t rev = pci_read_config_1(dev, PCI_USBREV);
  const char *str = NULL;

  if (rev == PCI_USB_REV_PRE_1_0)
    str = "pre-revision 1.0";
  else if (rev == PCI_USB_REV_1_0)
    str = "revision 1.0";
  else if (rev == PCI_USB_REV_1_1)
    str = "revision 1.1";
  else
    return 1;

  klog("host controller compliant with USB %s", str);
  return 0;
}

static int uhci_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);

  if (!pcid)
    return 0;

  if (pcid->class_code != PCI_USB_CLASSCODE ||
      pcid->subclass_code != PCI_USB_SUBCLASSCODE)
    return 0;

  if (pcid->progif != PCI_INTERFACE_UHCI)
    return 0;

  if (uhci_print_usb_release(dev))
    return 0;

  return 1;
}

/* The UHCI_PORT_ONE bit of a UHCI port register is always read as 1. */
static bool uhci_detect_port(uhci_state_t *uhci, uint8_t pn) {
  if (!chkw(UHCI_PORTSC(pn), UHCI_PORTSC_ONE))
    return false;

  /* Try to clear the bit. */
  clrw(UHCI_PORTSC(pn), UHCI_PORTSC_ONE);
  if (!chkw(UHCI_PORTSC(pn), UHCI_PORTSC_ONE))
    return false;

  return true;
}

/* Check initial values of a specified UHCI port. */
static bool uhci_check_port(uhci_state_t *uhci, uint8_t pn) {
  return !chkw(UHCI_PORTSC(pn), UHCI_PORTSC_SUSP | UHCI_PORTSC_PR |
                                  UHCI_PORTSC_RD | UHCI_PORTSC_LS |
                                  UHCI_PORTSC_POEDC | UHCI_PORTSC_PE);
}

/* Check whether a device is attached to a specified UHCI port. */
static bool uhci_device_present(uhci_state_t *uhci, uint8_t pn) {
  return chkw(UHCI_PORTSC(pn), UHCI_PORTSC_CCS);
}

static void uhci_reset_port(uhci_state_t *uhci, uint8_t pn) {
  setw(UHCI_PORTSC(pn), UHCI_PORTSC_PR);
  mdelay(50);
  clrw(UHCI_PORTSC(pn), UHCI_PORTSC_PR);
  mdelay(10);

  /* Only enable the port if some device is attached. */
  if (!uhci_device_present(uhci, pn))
    return;

  /* Clear status chenge indicators. */
  wclrw(UHCI_PORTSC(pn), UHCI_PORTSC_POEDC | UHCI_PORTSC_CSC);

  /* Enable the port. */
  while (!chkw(UHCI_PORTSC(pn), UHCI_PORTSC_PE))
    setw(UHCI_PORTSC(pn), UHCI_PORTSC_PE);
}

/* Obtain the number of roothub ports and reset each of them. */
static uint8_t uhci_init_ports(uhci_state_t *uhci) {
  uint8_t pn = 0;

  for (; uhci_detect_port(uhci, pn); pn++) {
    if (!uhci_check_port(uhci, pn))
      return 0;
    uhci_reset_port(uhci, pn);
  }

  uhci->nports = pn;
  klog("detected %hhu ports", uhci->nports);

  return pn;
}

static uint8_t uhci_number_of_ports(uhci_state_t *uhci) {
  return uhci->nports;
}

/* Obtain the physical address corresponding to a specified virtual address. */
static uint32_t uhci_paddr(void *vaddr) {
  paddr_t paddr = 0;
  pmap_kextract((vaddr_t)vaddr, &paddr);
  assert(paddr);
  /* UHCI only uses 32-bit addresses. */
  assert(paddr == (uint32_t)paddr);
  return (uint32_t)paddr;
}

static void qh_init(uhci_qh_t *qh) {
  bzero(qh, sizeof(uhci_qh_t));
  qh->qh_h_next = UHCI_PTR_T;
  qh->qh_e_next = UHCI_PTR_T;
}

static inline void qh_add_td(uhci_qh_t *qh, uhci_td_t *td) {
  qh->qh_e_next = uhci_paddr(td) | UHCI_PTR_TD;
}

static inline void qh_add_qh(uhci_qh_t *qh1, uhci_qh_t *qh2) {
  qh1->qh_e_next = uhci_paddr(qh2) | UHCI_PTR_QH;
}

static inline void qh_chain(uhci_qh_t *qh1, uhci_qh_t *qh2) {
  qh1->qh_h_next = uhci_paddr(qh2) | UHCI_PTR_QH;
}

static inline void qh_halt(uhci_qh_t *qh) {
  qh->qh_e_next |= UHCI_PTR_T;
  /* (MichalBlk) we should buisy wait 1 ms at this point. */
}

static inline void qh_unhalt(uhci_qh_t *qh) {
  qh->qh_e_next &= ~UHCI_PTR_T;
}

static void qh_insert(uhci_qh_t *mq, uhci_qh_t *qh) {
  WITH_SPIN_LOCK (&mq->qh_lock) {
    uhci_qh_t *last = TAILQ_LAST(&mq->qh_list, qh_list);

    TAILQ_INSERT_TAIL(&mq->qh_list, qh, qh_link);

    /* If we're inserting the first queue,
     * then we have to enable the main queue. */
    if (last)
      qh_chain(last, qh);
    else
      qh_add_qh(mq, qh);
  }
}

/* Schedule a queue for execution in `flr(log(interval))` ms. */
static void qh_schedule(uhci_state_t *uhci, uhci_qh_t *qh, uint8_t interval) {
  if (interval) {
    /* Compute flr(log(interval)). */
    for (int i = 1; i < 8; i <<= 1)
      interval |= interval >> i;
    interval = log2((interval + 1) >> 1);
  }

  /* Select a main queue to accommodate the request. */
  uint8_t qn = min(interval, UHCI_NQUEUES - 1);
  uhci_qh_t *mq = uhci->queues[qn];
  qh_insert(mq, qh);
}

static void qh_remove(uhci_qh_t *mq, uhci_qh_t *qh) {
  assert(spin_owned(&mq->qh_lock));

  uhci_qh_t *prev = TAILQ_PREV(qh, qh_list, qh_link);

  TAILQ_REMOVE(&mq->qh_list, qh, qh_link);

  /* If we're removing the last queue,
   * then we need to disable the main queue. */
  if (prev)
    prev->qh_h_next = qh->qh_h_next;
  else
    mq->qh_e_next = UHCI_PTR_T;
}

/* Allocate continuous physical memory for stable UHCI usage. */
static void *uhci_alloc_pmem(size_t size) {
  size_t rsize = roundup(size, PAGESIZE);
  vm_page_t *pages = vm_page_alloc(rsize);
  assert(pages);
  void *va = (void *)kmem_map(pages->paddr, rsize, PMAP_NOCACHE);
  assert(va);
  return va;
}

static void uhci_init_queues(uhci_state_t *uhci) {
  uhci_qh_t *mqs = uhci_alloc_pmem(UHCI_NQUEUES * sizeof(uhci_qh_t));

  /* Organize the main queues in a list. */
  for (int i = 0; i < UHCI_NQUEUES; i++) {
    qh_init(&mqs[i]);
    spin_init(&mqs[i].qh_lock, 0);
    qh_chain(&mqs[i], &mqs[i - 1]);
    TAILQ_INIT(&mqs[i].qh_list);
    uhci->queues[i] = &mqs[i];
  }
  /* Revise the boundary case. */
  mqs[0].qh_h_next = UHCI_PTR_T;
}

static void uhci_init_frames(uhci_state_t *uhci) {
  uhci->frames = uhci_alloc_pmem(UHCI_FRAMELIST_COUNT * sizeof(uint32_t));

  /* Set each frame pointer to an appropriate main queue. */
  for (int i = 0; i < UHCI_FRAMELIST_COUNT; i++) {
    int maxpow2 = ffs(i + 1);
    int qn = min(maxpow2, UHCI_NQUEUES) - 1;
    uhci->frames[i] = uhci_paddr(uhci->queues[qn]) | UHCI_PTR_QH;
  }

  /* Set frame list base address. */
  outd(UHCI_FLBASEADDR, uhci_paddr(uhci->frames));
}

static inline bool buf_free(void *buf) {
  return *(uint32_t *)buf & UHCI_BUF_FREE;
}

static inline void buf_mark_free(void *buf) {
  *(uint32_t *)buf |= UHCI_BUF_FREE;
}

static inline void buf_mark_allocated(void *buf) {
  *(uint32_t *)buf &= ~UHCI_BUF_FREE;
}

static inline void *buf_payload(void *buf) {
  return buf + max(UHCI_QH_ALIGN, UHCI_TD_ALIGN);
}

static inline void *buf_block(void *payload) {
  return payload - max(UHCI_QH_ALIGN, UHCI_TD_ALIGN);
}

static void uhci_init_pool(uhci_state_t *uhci) {
  uhci->pool = uhci_alloc_pmem(PAGESIZE);

  /* Designate each buffer as free. */
  void *buf = uhci->pool;
  for (int i = 0; i < UHCI_BUF_NUM; i++, buf += UHCI_BUF_SIZE)
    buf_mark_free(buf);

  spin_init(&uhci->lock, 0);
}

static void *uhci_alloc_pool(uhci_state_t *uhci) {
  SCOPED_SPIN_LOCK(&uhci->lock);

  void *buf = uhci->pool;

  /* First fit. */
  for (int i = 0; i < UHCI_BUF_NUM; i++, buf += UHCI_BUF_SIZE)
    if (buf_free(buf)) {
      buf_mark_allocated(buf);
      return buf_payload(buf);
    }

  return NULL;
}

static void uhci_free_pool(uhci_state_t *uhci, void *buf) {
  SCOPED_SPIN_LOCK(&uhci->lock);
  void *block = buf_block(buf);
  assert(!buf_free(block));
  buf_mark_free(block);
}

static void td_init(uhci_td_t *td, uint32_t ls, uint32_t ioc, uint32_t token,
                    void *data) {
  bzero(td, sizeof(uhci_td_t));
  td->td_next =
    (ioc ? UHCI_PTR_T : uhci_paddr(td + 1) | UHCI_PTR_VF | UHCI_PTR_TD);
  td->td_status = UHCI_TD_SET_ERRCNT(3) | ioc | ls | UHCI_TD_ACTIVE;
  td->td_token = token;
  td->td_buffer = (data ? uhci_paddr(data) : 0);
}

#define td_setup(td, ls, e, d, dt)                                             \
  td_init((td), (ls), 0,                                                       \
          UHCI_TD_SETUP(sizeof(usb_device_request_t), (e), (d)), (dt))

#define td_token(ln, e, d, tg, tp)                                             \
  (((tp)&TF_INPUT) ? UHCI_TD_IN((ln), (e), (d), (tg))                          \
                   : UHCI_TD_OUT((ln), (e), (d), (tg)))

#define td_data(td, ls, ln, e, d, tg, dt, tp)                                  \
  td_init((td), (ls), 0, td_token((ln), (e), (d), (tg), (tp)), (dt))

#define td_status(td, ls, e, d, tp)                                            \
  td_init((td), (ls), UHCI_TD_IOC, td_token(0, (e), (d), 1, (tp)), NULL)

static inline uint32_t td_error_status(uhci_td_t *td) {
  return td->td_status & UHCI_TD_ERROR;
}

static inline bool td_active(uhci_td_t *td) {
  return td->td_status & UHCI_TD_ACTIVE;
}

static void td_activate(uhci_td_t *td) {
  uint32_t ioc = td->td_status & UHCI_TD_IOC;
  uint32_t ls = td->td_status & UHCI_TD_LS;
  td->td_status = UHCI_TD_SET_ERRCNT(3) | ioc | ls | UHCI_TD_ACTIVE;
}

static inline bool td_last(uhci_td_t *td) {
  return td->td_next & UHCI_PTR_T;
}

static void qh_process(uhci_state_t *uhci, uhci_qh_t *mq, uhci_qh_t *qh) {
  assert(spin_owned(&mq->qh_lock));

  usb_buf_t *usbb = qh->qh_usbb;
  uhci_td_t *first = (uhci_td_t *)(qh + 1);
  uhci_td_t *td = first;
  int poll = usbb->flags & TF_INTERRUPT;
  uint32_t error = 0;

  for (; !error && !td_active(td); td++) {
    error |= td_error_status(td);
    if (td_last(td))
      break;
    if (poll)
      td_activate(td);
  }

  if (td_last(td) && !td_active(td)) {
    if (!error && poll)
      td_activate(td);
  } else if (!error) {
    /* Return if this queue didn't cause the interrupt. */
    return;
  }

  void *data = NULL;
  transfer_flags_t flags = 0;

  if (!error) {
    int control = usbb->flags & TF_CONTROL;
    data = (void *)(td + 1) + (control ? sizeof(usb_device_request_t) : 0);
  } else {
    int stalled = error & UHCI_TD_STALLED;
    int other = error & ~UHCI_TD_STALLED;
    flags = (stalled ? TF_STALLED : 0) | (other ? TF_ERROR : 0);
  }
  usb_process(usbb, data, flags);

  if (error || !poll) {
    qh_remove(mq, qh);
    uhci_free_pool(uhci, qh);
  } else {
    qh_add_td(qh, first);
  }
}

static intr_filter_t uhci_isr(void *data) {
  uhci_state_t *uhci = data;
  uint16_t intrmask = UHCI_STS_USBINT | UHCI_STS_USBEI;

  if (!chkw(UHCI_STS, intrmask))
    return IF_STRAY;

  /* Clear the interrupt flags. */
  wclrw(UHCI_STS, intrmask);

  for (int i = 0; i < UHCI_NQUEUES; i++) {
    uhci_qh_t *mq = uhci->queues[i];
    qh_halt(mq);

    WITH_SPIN_LOCK (&mq->qh_lock) {
      uhci_qh_t *qh, *next;
      TAILQ_FOREACH_SAFE (qh, &mq->qh_list, qh_link, next)
        qh_process(uhci, mq, qh);
      if (!TAILQ_EMPTY(&mq->qh_list))
        qh_unhalt(mq);
    }
  }

  return IF_FILTERED;
}

/* Obtain the size of structures involved to compose a request. */
static size_t uhci_req_size(uint16_t mps, usb_buf_t *usbb) {
  int cntr_size = (usbb->flags & TF_CONTROL ? 2 : 0); /* setup + status */
  return sizeof(uhci_qh_t) +
         ((roundup(usbb->transfer_size, mps) / mps + cntr_size) *
          sizeof(uhci_td_t));
}

static void uhci_transfer(uhci_state_t *uhci, usb_device_t *usbd,
                          usb_buf_t *usbb, usb_device_request_t *req) {
  uint16_t transfer_size = usbb->transfer_size;
  uint16_t mps = usb_max_pkt_size(usbd, usbb);
  size_t offset = uhci_req_size(mps, usbb);
  uint8_t endp = usb_endp_addr(usbd, usbb);
  void *buf = uhci_alloc_pool(uhci);
  uhci_qh_t *qh = buf;
  uhci_td_t *td = (uhci_td_t *)(qh + 1);

  qh_init(qh);
  qh->qh_usbb = usbb;
  qh_add_td(qh, td);

  /* Are we talking with a low speed device? */
  uint32_t ls = chkw(UHCI_PORTSC(usbd->port), UHCI_PORTSC_LSDA);

  void *dt = buf + offset;
  uint16_t cnt = 0;

  if (usbb->flags & TF_CONTROL) {
    /* Copyin the request. */
    usb_device_request_t *r = dt;
    memcpy(r, req, sizeof(usb_device_request_t));

    /* Data starts immediately after the request. */
    dt = r + 1;

    /* Prepare a SETUP packet. */
    td_setup(td, ls, endp, usbd->addr, r);
    cnt++;
  }

  /* Copyin provided data. */
  if (!(usbb->flags & TF_INPUT))
    memcpy(dt, usbb->buf.data, transfer_size);

  /* Prepare DATA packets. */
  for (uint16_t nbytes = 0; nbytes != transfer_size; cnt++) {
    uint16_t psize = min(transfer_size - nbytes, mps);
    td_data(&td[cnt], ls, psize, endp, usbd->addr, cnt & 1, dt, usbb->flags);
    nbytes += psize;
    dt += psize;
  }

  if (usbb->flags & TF_CONTROL) {
    /* Prepare a STATUS packet. */
    uint8_t sts_type = usb_status_type(usbb);
    td_status(&td[cnt], ls, endp, usbd->addr, sts_type);
  } else {
    /* The last TD has to finish the transfer. */
    uhci_td_t *last = &td[cnt - 1];
    last->td_next = UHCI_PTR_T;
    last->td_status |= UHCI_TD_IOC;
  }

  qh_schedule(uhci, qh, usb_interval(usbd, usbb));
}

DEVCLASS_CREATE(usbhc);
static usbhc_space_t uhci_space;

static int uhci_attach(device_t *dev) {
  uhci_state_t *uhci = dev->state;

  uhci->regs = device_take_ioports(dev, 4, RF_ACTIVE);
  uhci->irq = device_take_irq(dev, 0, RF_ACTIVE);
  assert(uhci->regs);
  assert(uhci->irq);

  bus_intr_setup(dev, uhci->irq, uhci_isr, NULL, uhci, "UHCI");

  /* Perform the global reset of the UHCI controller. */
  setw(UHCI_CMD, UHCI_CMD_GRESET);
  mdelay(50);
  setw(UHCI_CMD, 0x0000);
  mdelay(100);

  /* After global reset, all registers are set to their
   * default values. */
  if (inw(UHCI_CMD) != 0x0000)
    return 1;
  if (inw(UHCI_STS) != UHCI_STS_HCH)
    return 1;
  if (inb(UHCI_SOF) != 0x40) /* each frame is 1 ms */
    return 1;

  /* Unhalt the controller. */
  wclrw(UHCI_STS, UHCI_STS_HCH);

  /* Perform the host controller reset. */
  setw(UHCI_CMD, UHCI_CMD_HCRESET);
  while (inw(UHCI_CMD) & UHCI_CMD_HCRESET)
    ;

  uhci_init_queues(uhci);
  uhci_init_frames(uhci);
  uhci_init_pool(uhci);

  if (!uhci_init_ports(uhci))
    return 1;

  /* Enable bus master mode. */
  pci_enable_busmaster(dev);

  /* Set current frame number to the first frame pointer. */
  outw(UHCI_FRNUM, 0x0000);

  /* Turn on the IOC and error interrupts. */
  setw(UHCI_INTR, UHCI_INTR_TOCRCIE | UHCI_INTR_IOCE);

  /* Start the controller. */
  outw(UHCI_CMD, UHCI_CMD_RS);

  /* Register the UHCI host controller space. */
  USBHC_SPACE_REGISTER(uhci_space);
  USBHC_SPACE_SET_STATE(uhci);

  dev->devclass = &DEVCLASS(usbhc);

  /* Detect and configure attached devices. */
  usb_enumerate(dev);

  return bus_generic_probe(dev);
}

static usbhc_space_t uhci_space = {
  USBHC_SPACE_SET(number_of_ports, uhci_number_of_ports),
  USBHC_SPACE_SET(device_present, uhci_device_present),
  USBHC_SPACE_SET(reset_port, uhci_reset_port),
  USBHC_SPACE_SET(transfer, uhci_transfer),
};

static driver_t uhci = {
  .desc = "UHCI driver",
  .size = sizeof(uhci_state_t),
  .pass = SECOND_PASS,
  .probe = uhci_probe,
  .attach = uhci_attach,
};

DEVCLASS_ENTRY(pci, uhci);
