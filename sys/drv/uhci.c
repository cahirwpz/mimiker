/*
 * UHCI host controller PCI deriver.
 *
 * For explanation of terms used throughout the code
 * please see the following documents:
 * - ftp://ftp.netbsd.org/pub/NetBSD/misc/blymn/uhci11d.pdf
 * - http://esd.cs.ucr.edu/webres/usb11.pdf
 */
#define KL_LOG KL_DEV
#include <sys/errno.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/libkern.h>
#include <sys/klog.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/pool.h>
#include <sys/time.h>
#include <dev/pci.h>
#include <dev/usb.h>
#include <dev/usbhc.h>
#include <dev/uhci.h>
#include <dev/uhcireg.h>

/* Number of main queues.
 * Available shedule intervals are: 1, 2, ..., 2^(`UHCI_NMAINQS` - 1). */
#define UHCI_NMAINQS 5
#define UHCI_MAX_INTERVAL (1 << (UHCI_NMAINQS - 1))

typedef struct uhci_state {
  uhci_qh_t *mainqs[UHCI_NMAINQS]; /* main queues (i.e. schedule queues) */
  uint32_t *frames;                /* UHCI frame list */
  resource_t *regs;                /* host controller registers */
  resource_t *irq;                 /* host controller interrupt */
  uint8_t nports;                  /* number of roothub ports */
} uhci_state_t;

/*
 * For each transfer we need a contiguous memory range to compose appropriate
 * UCHI specific structures which state a USB transaction. Besides that,
 * a transfer also needs another contiguous span for I/O data to transfer.
 * To meet these requirements we supply two pools:
 * - `P_TFR`  - memory buffers for UHCI transfer constructs,
 * - `P_DATA` - memory buffers for I/O data to transfer.
 */

#define UHCI_NBUFFERS 8

#define UHCI_TFR_BUF_SIZE (sizeof(uhci_qh_t) + 64 * sizeof(uhci_td_t))
#define UHCI_TFR_POOL_SIZE (UHCI_NBUFFERS * UHCI_TFR_BUF_SIZE)

#define UHCI_DATA_BUF_SIZE UHCI_TD_MAXLEN
#define UHCI_DATA_POOL_SIZE (UHCI_NBUFFERS * UHCI_DATA_BUF_SIZE)

#define UHCI_ALIGNMENT max(UHCI_TD_ALIGN, UHCI_QH_ALIGN)

static POOL_DEFINE(P_TFR, "UHCI transfer buffers", UHCI_TFR_BUF_SIZE);
static POOL_DEFINE(P_DATA, "UHCI data  buffers", UHCI_DATA_BUF_SIZE);

/*
 * How do we build a UHCI transfer?
 *
 * For each transfer we need a buffer for UHCI constructs and a buffer
 * for I/O data. The former contains a single UHCI queue followed by
 * a number of UHCI transfer descriptors. These describe a UHCI transfer.
 * Each transfer descriptor contains a pointer which points to some location
 * in the I/O data buffer.
 *
 * Conceptual drawing:
 *
 *         transfer buffer                           data I/O buffer
 *        (UHCI constructs)                 (buffer for data/bytes to transfer)
 *                              `qh_data`
 *   start ---------------           -------> start ---------------
 *         |             |          /               |             |
 *         |    queue    |---------/                |             |
 *         |             |                          |             |
 *         ---------------                          |             |
 *         |   transfer  |      `td_buffer`         |             |
 *         |  descriptor |------------------------> |             |
 *         |      0      |                          |  row data   |
 *         ---------------                          |   stream    |
 *         |             |                          |             |
 *               ...                                      ...
 *         |             |                          |             |
 *         ---------------      `td_buffer`         |             |
 *         |   transfer  |         ---------------> |             |
 *         |  descriptor |--------/                 |             |
 *         |      n      |                          |             |
 *     end ---------------                          ---------------
 */

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
#define chkb(p, b) ((inb(p) & (b)) != 0)
#define chkw(p, b) ((inw(p) & (b)) != 0)
#define chkd(p, b) ((ind(p) & (b)) != 0)

/*
 * Helper functions.
 */

/* Obtain the physical address corresponding
 * to the specified virtual address. */
static uhci_physaddr_t uhci_physaddr(void *vaddr) {
  paddr_t paddr = 0;
  pmap_kextract((vaddr_t)vaddr, &paddr);
  return (uhci_physaddr_t)paddr;
}

/* Allocate and map required amount of physical memory.
 * This is used only in the attach phase. */
static void *alloc_contig(size_t insize, size_t *outsize) {
  /* Our contiguous memory allocation interface permits only sizes of
   * the following form: 2^k * PAGESIZE. */
  assert(insize);
  insize = max(insize, (size_t)PAGESIZE);
  insize = 1UL << log2(insize);
  paddr_t paddr;
  vaddr_t vaddr = kmem_alloc_contig(&paddr, insize, PMAP_NOCACHE);
  /* XXX: in case of more than 4GB RAM,
   * we should somehow ensure the following. */
  assert(vaddr == (uhci_physaddr_t)vaddr);
  if (outsize)
    *outsize = insize;
  return (void *)vaddr;
}

/*
 * Transfer descriptor handling functions.
 */

/*
 * Initialize a transfer descriptor.
 *
 * - `ls`  - `true` in case of a low speed transfer, `false` otherwise.
 * - `ioc` - (Interrupt On Completion) `true` if it's the last transfer
 *           descriptor in a transfer, `false` otherwise.
 * - `token` - one of `UHCI_TD_{SETUP,OUT,IN}`.
 * - `data`  - virtual address of an I/O buffer.
 */
static void td_init(uhci_td_t *td, bool ls, bool ioc, uint32_t token,
                    void *data) {
  uint32_t ls_mask = (ls ? UHCI_TD_LS : 0);
  uint32_t ioc_mask = (ioc ? UHCI_TD_IOC : 0);

  bzero(td, sizeof(uhci_td_t));
  /* Consecutive transfer descriptors are always contiguou. */
  td->td_next =
    (ioc ? UHCI_PTR_T : uhci_physaddr(td + 1) | UHCI_PTR_VF | UHCI_PTR_TD);
  td->td_status = UHCI_TD_SET_ERRCNT(3) | ioc_mask | ls_mask | UHCI_TD_ACTIVE;
  td->td_token = token;
  td->td_buffer = (data ? uhci_physaddr(data) : 0);
}

/* Create a SETUP transfer descriptor. */
static void td_setup(uhci_td_t *td, bool ls, uint8_t addr,
                     usb_device_request_t *req) {
  /* SETUP packages are used only in control transfers, hence endpoint is 0. */
  uint32_t token = UHCI_TD_SETUP(sizeof(usb_device_request_t), 0, addr);
  /* A SETUP packet is always the first packet of a transfer, thus
   * no Interrupt On Competion is used. */
  td_init(td, ls, false, token, req);
}

/* Compose a DATA IN/DATA OUT token. */
static uint32_t td_data_token(uint16_t len, uint8_t endp, uint8_t addr,
                              uint8_t data_toggle, usb_direction_t dir) {
  if (dir == USB_DIR_INPUT)
    return UHCI_TD_IN(len, endp, addr, data_toggle);
  assert(dir == USB_DIR_OUTPUT);
  return UHCI_TD_OUT(len, endp, addr, data_toggle);
}

/* Create a DATA IN/DATA OUT transfer descriptor. */
static void td_data(uhci_td_t *td, bool ls, bool ioc, uint16_t len,
                    uint8_t endp, uint8_t addr, uint8_t data_toggle,
                    usb_direction_t dir, void *data) {
  uint32_t token = td_data_token(len, endp, addr, data_toggle, dir);
  td_init(td, ls, ioc, token, data);
}

/* Create a STATUS transfer descriptor. */
static void td_status(uhci_td_t *td, uint32_t ls, uint8_t addr,
                      usb_direction_t dir) {
  /* SETUP packages are used only in control transfers, hence endpoint is 0.
   * A SETUP packet is always the last packet of a transfer, thus
   * an Interrupt On Competion shuld be triggered at the end. */
  td_data(td, ls, true /* ioc */, 0 /* len */, 0 /* endp */, addr,
          1 /* data_toggle */, dir, NULL /* data */);
}

/* Get transfer descriptor error status. */
static inline uint32_t td_error_status(uhci_td_t *td) {
  return td->td_status & UHCI_TD_ERROR;
}

/* Check whether a transfer descriptro is active (i.e. should be executed). */
static inline bool td_active(uhci_td_t *td) {
  return td->td_status & UHCI_TD_ACTIVE;
}

/* Discard all errors and sign a transfer descriptor as to be executed. */
static void td_reactivate(uhci_td_t *td) {
  uint32_t ioc = td->td_status & UHCI_TD_IOC;
  uint32_t ls = td->td_status & UHCI_TD_LS;
  td->td_status = UHCI_TD_SET_ERRCNT(3) | ioc | ls | UHCI_TD_ACTIVE;
}

/* Chect whether a transfer descriptor is the last one in a transfer. */
static inline bool td_last(uhci_td_t *td) {
  return td->td_next & UHCI_PTR_T;
}

/*
 * Queue handling functions.
 */

static inline void qh_add_td(uhci_qh_t *qh, uhci_td_t *td);
static inline uhci_td_t *qh_first_td(uhci_qh_t *qh);

/* Initialize a regular (i.e. not main) queue. */
static void qh_init(uhci_qh_t *qh, usb_buf_t *buf) {
  qh->qh_h_next = UHCI_PTR_T;
  qh->qh_e_next = UHCI_PTR_T;
  qh->qh_buf = buf;
  qh_add_td(qh, qh_first_td(qh));
}

/* Allocate a new queue (including transfer I/O data buffer). */
static uhci_qh_t *qh_alloc(usb_buf_t *buf) {
  void *area = pool_alloc(P_TFR, M_ZERO);
  /* Allocation scheme of pool's memory ensures that adjusting the
   * virtual address also adjusts the physicall address. */
  uhci_qh_t *qh = align(area, UHCI_ALIGNMENT);
  qh_init(qh, buf);
  qh->qh_cookie = area;
  qh->qh_data = pool_alloc(P_DATA, M_ZERO);
  return qh;
}

/* Release a queue (including transfer I/O data buffer). */
static void qh_free(uhci_qh_t *qh) {
  void *data = qh->qh_data;
  void *area = qh->qh_cookie;
  assert(data);
  pool_free(P_DATA, data);
  pool_free(P_TFR, area);
}

/* Initialize a main queue. */
static void qh_init_main(uhci_qh_t *mq) {
  bzero(mq, sizeof(uhci_qh_t));
  mq->qh_h_next = UHCI_PTR_T;
  mq->qh_e_next = UHCI_PTR_T;
  spin_init(&mq->qh_lock, 0);
  TAILQ_INIT(&mq->qh_list);
}

/* Point the queue's element link pointer
 * at the specified transfer descriptor. */
static inline void qh_add_td(uhci_qh_t *qh, uhci_td_t *td) {
  qh->qh_e_next = uhci_physaddr(td) | UHCI_PTR_TD;
}

/* Return a pointer to the first transfer descriptor composing
 * the specified queue. */
static inline uhci_td_t *qh_first_td(uhci_qh_t *qh) {
  return (uhci_td_t *)(qh + 1);
}

/* Point the queue's element link pointer at the specified queue. */
static inline void qh_add_qh(uhci_qh_t *qh1, uhci_qh_t *qh2) {
  qh1->qh_e_next = uhci_physaddr(qh2) | UHCI_PTR_QH;
}

/* Point the queue's head link pointer at the specified queue. */
static inline void qh_chain(uhci_qh_t *qh1, uhci_qh_t *qh2) {
  qh1->qh_h_next = uhci_physaddr(qh2) | UHCI_PTR_QH;
}

/* Tell the controller to omit the specified queue from now on. */
static inline void qh_halt(uhci_qh_t *qh) {
  qh->qh_e_next |= UHCI_PTR_T;
}

/* Tell the controller to continue executing the specified queue when
 * the queue is encountered. */
static inline void qh_unhalt(uhci_qh_t *qh) {
  qh->qh_e_next &= ~UHCI_PTR_T;
}

/* Insert the specified queue into the specified main queue for execution. */
static void qh_insert(uhci_qh_t *mq, uhci_qh_t *qh) {
  WITH_SPIN_LOCK (&mq->qh_lock) {
    uhci_qh_t *last = TAILQ_LAST(&mq->qh_list, qh_list);

    TAILQ_INSERT_TAIL(&mq->qh_list, qh, qh_link);

    /* If we're inserting the first queue,
     * then we have to enable the main queue. */
    if (!last)
      qh_add_qh(mq, qh);
    else
      qh_chain(last, qh);
  }
}

/* Remove the specified queue from the specified main queue. */
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

/* Obtain the queue's status. If an error has encountered, `error` is
 * set appropriately, otherwise, `true` is returned if the queue
 * has been executed. */
static bool qh_status(uhci_qh_t *qh, usb_error_t *error) {
  uhci_td_t *td = qh_first_td(qh);
  uint32_t err = 0;

  do {
    err |= td_error_status(td);
  } while (!td_last(td++));

  *error = err;
  return !td_active(td - 1);
}

/* Remove the specified queue from the pointed main queue and reclaim
 * UHCI buffers associated with the queue. */
static void qh_discard(uhci_qh_t *mq, uhci_qh_t *qh) {
  assert(spin_owned(&mq->qh_lock));
  qh_remove(mq, qh);
  qh_free(qh);
}

/* Set a queue to be executed. */
static void qh_reactivate(uhci_qh_t *qh) {
  uhci_td_t *td = qh_first_td(qh);
  qh_add_td(qh, td);
  do {
    td_reactivate(td);
  } while (!td_last(td++));
}

/*
 * UHCI transfer handling functions.
 */

/* Convert host controller error flags into USB bus error flags. */
static usb_error_t uhcie2usbe(uint32_t error) {
  usb_error_t uerr = 0;
  /* FTTB, we only distinguish a STALL condition. */
  if (error & UHCI_TD_STALLED)
    uerr |= USB_ERR_STALLED;
  /* Signal any other errors. */
  error &= ~UHCI_TD_STALLED;
  if (error)
    uerr |= USB_ERR_OTHER;
  return uerr;
}

/* Process the transfer identified by queue `qh`. */
static void uhci_process(uhci_state_t *uhci, uhci_qh_t *mq, uhci_qh_t *qh) {
  assert(spin_owned(&mq->qh_lock));

  qh_halt(qh);

  usb_buf_t *buf = qh->qh_buf;

  /* Obtain the queue's status. */
  uint32_t error;
  bool executed = qh_status(qh, &error);

  /* If an error has occured, signal the error and discard the transfer. */
  if (error) {
    usb_error_t uerr = uhcie2usbe(error);
    usb_process(buf, NULL, uerr);
    qh_discard(mq, qh);
    return;
  }

  /* If the transfer isn't done yet, we'll come back to it later. */
  if (!executed) {
    qh_unhalt(qh);
    return;
  }

  /* Let the USB bus layer handle the received data. */
  usb_process(buf, qh->qh_data, 0);

  /* If this is a periodic transfer we have to reactivate it. */
  if (usb_buf_periodic(buf)) {
    /* Adding a transfer descriptor will unhalt the queue automatically. */
    qh_reactivate(qh);
  } else {
    qh_discard(mq, qh);
  }
}

/* UHCI Interrupt Service Routine. */
static intr_filter_t uhci_isr(void *data) {
  uhci_state_t *uhci = data;
  /* FTTB, we're interested in IOC and error events. */
  uint16_t intrmask = UHCI_STS_USBINT | UHCI_STS_USBEI;

  /* Sould we bother this? */
  if (!chkw(UHCI_STS, intrmask))
    return IF_STRAY;

  /* Clear the interrupt flags so that new requests can be queued. */
  wclrw(UHCI_STS, intrmask);

  for (int i = 0; i < UHCI_NMAINQS; i++) {
    uhci_qh_t *mq = uhci->mainqs[i];
    qh_halt(mq);

    /* Travers each main queue to find the delinquent. */
    WITH_SPIN_LOCK (&mq->qh_lock) {
      uhci_qh_t *qh, *next;
      TAILQ_FOREACH_SAFE (qh, &mq->qh_list, qh_link, next)
        uhci_process(uhci, mq, qh);
      /* Unhalt only non-empty main queues. */
      if (!TAILQ_EMPTY(&mq->qh_list))
        qh_unhalt(mq);
    }
  }

  return IF_FILTERED;
}

/* Schedule a queue for execution in `flr(log(interval))` ms. */
static void uhci_schedule(uhci_state_t *uhci, uhci_qh_t *qh, uint8_t interval) {
  if (interval) {
    /* Compute flr(log(interval)). */
    for (int i = 1; i < 8; i <<= 1)
      interval |= interval >> i;
    interval = log2((interval + 1) >> 1);
  }

  /* Select a main queue to accommodate the request. */
  uint8_t qn = min(interval, UHCI_NMAINQS - 1);
  uhci_qh_t *mq = uhci->mainqs[qn];
  qh_insert(mq, qh);
}

/*
 * Setup the data stage of a transaction.
 *
 * - `maxpkt`, `addr`, `endp` - see `include/dev/usbhc.h`,
 * - `transfer_size` - number of bytes to transfer in the data stage,
 * - `dir` - transfer direction,
 * - `ls`  - see `td_init`,
 * - `td`  - address of the first transfer descriptor to fill,
 * - `toggle` - firs data toggle to grant,
 * - `data`   - pointer to the I/O data.
 *
 * Returns pointer to the next available transfer descriptor.
 */
static uhci_td_t *uhci_data_stage(uint16_t maxpkt, uint8_t addr, uint8_t endp,
                                  usb_buf_t *buf, bool ls, uhci_td_t *td,
                                  uint16_t toggle, void *data) {
  usb_direction_t dir = buf->dir;
  uint16_t transfer_size = buf->transfer_size;

  /* Copyin data to transfer. */
  if (dir == USB_DIR_OUTPUT)
    usb_buf_copy_data(buf, data);

  /* Prepare DATA packets. */
  for (uint16_t nbytes = 0; nbytes != transfer_size; toggle ^= 1) {
    uint16_t pktsize = min(transfer_size - nbytes, maxpkt);
    td_data(td++, ls, false, pktsize, endp, addr, toggle, dir, data);
    nbytes += pktsize;
    data += pktsize;
  }

  return td;
}

static inline bool uhci_low_speed(uhci_state_t *uhci, uint8_t port);

/* Issue a control transfer. */
static void uhci_control_transfer(device_t *dev, uint16_t maxpkt, uint8_t port,
                                  uint8_t addr, usb_buf_t *buf,
                                  usb_device_request_t *req) {
  assert(buf->transfer_size + sizeof(usb_device_request_t) <=
         UHCI_DATA_BUF_SIZE);

  uhci_state_t *uhci = dev->parent->state;
  bool ls = uhci_low_speed(uhci, port);
  uhci_qh_t *qh = qh_alloc(buf);

  /* Copyin the USB device request at the end of the data buffer. */
  void *req_cpy = qh->qh_data + buf->transfer_size;
  memcpy(req_cpy, req, sizeof(usb_device_request_t));

  /* Prepare a SETUP packet. */
  uhci_td_t *td = qh_first_td(qh);
  td_setup(td, ls, addr, req_cpy);

  /* Preapre the data stage. */
  td = uhci_data_stage(maxpkt, addr, 0, buf, ls, td + 1, 1, qh->qh_data);

  /* Prepare a STATUS packet. */
  usb_direction_t status_dir = usb_buf_status_direction(buf);
  td_status(td, ls, addr, status_dir);

  uhci_schedule(uhci, qh, 0);
}

/* Issue an interrupt transfer. */
static void uhci_interrupt_transfer(device_t *dev, uint16_t maxpkt,
                                    uint8_t port, uint8_t addr, uint8_t endp,
                                    uint8_t interval, usb_buf_t *buf) {
  assert(buf->transfer_size <= UHCI_DATA_BUF_SIZE);

  uhci_state_t *uhci = dev->parent->state;
  bool ls = uhci_low_speed(uhci, port);
  uhci_qh_t *qh = qh_alloc(buf);

  /* Preapre the data stage. */
  uhci_td_t *td = uhci_data_stage(maxpkt, addr, endp, buf, ls, qh_first_td(qh),
                                  0, qh->qh_data);

  /* The last transfer descriptor has to finish the transfer. */
  td--;
  td->td_next = UHCI_PTR_T;
  td->td_status |= UHCI_TD_IOC;

  uhci_schedule(uhci, qh, interval);
}

/* Issue a bulk transfer. */
static void uhci_bulk_transfer(device_t *dev, uint16_t maxpkt, uint8_t port,
                               uint8_t addr, uint8_t endp, usb_buf_t *buf) {
  /* We set the interval to the maximum possible value since bulk
   * transfers aren't so urgent and common as other transfer types. */
  uhci_interrupt_transfer(dev, maxpkt, port, addr, endp, UHCI_MAX_INTERVAL,
                          buf);
}

/*
 * Software context initialization functions.
 */

/* Initialize main queues. */
static void uhci_init_mainqs(uhci_state_t *uhci) {
  uhci_qh_t *mqs = alloc_contig(UHCI_NMAINQS * sizeof(uhci_qh_t), NULL);

  /* Organize the main queues in a list. */
  for (int i = 0; i < UHCI_NMAINQS; i++) {
    qh_init_main(&mqs[i]);
    qh_chain(&mqs[i], &mqs[i - 1]);
    uhci->mainqs[i] = &mqs[i];
  }
  /* Revise the boundary case. */
  mqs[0].qh_h_next = UHCI_PTR_T;
}

/* Initialize the UHCI fram list. */
static void uhci_init_frames(uhci_state_t *uhci) {
  uhci->frames = alloc_contig(UHCI_FRAMELIST_COUNT * sizeof(uint32_t), NULL);

  /* Set each frame pointer to the appropriate main queue. */
  for (int i = 0; i < UHCI_FRAMELIST_COUNT; i++) {
    int maxpow2 = ffs(i + 1);
    int qn = min(maxpow2, UHCI_NMAINQS) - 1;
    uhci->frames[i] = uhci_physaddr(uhci->mainqs[qn]) | UHCI_PTR_QH;
  }

  /* Set frame list base address. */
  outd(UHCI_FLBASEADDR, uhci_physaddr(uhci->frames));
}

/* Supply a contiguous physical memory for further buffer allocation. */
static void uhci_init_pool(void) {
  size_t tfr_size;
  void *tfr_pool = alloc_contig(UHCI_TFR_POOL_SIZE, &tfr_size);
  pool_add_page(P_TFR, tfr_pool, tfr_size);

  size_t data_size;
  void *data_pool = alloc_contig(UHCI_DATA_POOL_SIZE, &data_size);
  pool_add_page(P_DATA, data_pool, data_size);
}

/*
 * UHCI host controller manage functions.
 */

/* The UHCI_PORTSC_ONE bit of a UHCI port register is always read as 1.
 * This provides a simple method to verify whether a memory word is
 * a root hub port status/control register. */
static bool uhci_is_port(uhci_state_t *uhci, uint8_t port) {
  if (!chkw(UHCI_PORTSC(port), UHCI_PORTSC_ONE))
    return false;

  /* Try to clear the bit. */
  clrw(UHCI_PORTSC(port), UHCI_PORTSC_ONE);
  if (!chkw(UHCI_PORTSC(port), UHCI_PORTSC_ONE))
    return false;

  return true;
}

/* Verify the initial values of a specified root hub port. */
static bool uhci_check_port(uhci_state_t *uhci, uint8_t port) {
  return !chkw(UHCI_PORTSC(port), UHCI_PORTSC_SUSP | UHCI_PORTSC_PR |
                                    UHCI_PORTSC_RD | UHCI_PORTSC_LS |
                                    UHCI_PORTSC_POEDC | UHCI_PORTSC_PE);
}

/* Check whether a device is attached to the specified root hub port. */
static bool uhci_device_present(device_t *dev, uint8_t port) {
  uhci_state_t *uhci = dev->state;
  return chkw(UHCI_PORTSC(port), UHCI_PORTSC_CCS);
}

/* Reset the specified root hub port. */
static void uhci_reset_port(device_t *dev, uint8_t port) {
  uhci_state_t *uhci = dev->state;

  setw(UHCI_PORTSC(port), UHCI_PORTSC_PR);
  mdelay(USB_PORT_ROOT_RESET_DELAY_SPEC);
  clrw(UHCI_PORTSC(port), UHCI_PORTSC_PR);
  mdelay(USB_PORT_RESET_RECOVERY_SPEC);

  /* Only enable the port if some device is attached. */
  if (!uhci_device_present(dev, port))
    return;

  /* Clear status chenge indicators. */
  wclrw(UHCI_PORTSC(port), UHCI_PORTSC_POEDC | UHCI_PORTSC_CSC);

  /* Enable the port. */
  while (!chkw(UHCI_PORTSC(port), UHCI_PORTSC_PE))
    setw(UHCI_PORTSC(port), UHCI_PORTSC_PE);
}

/* Obtain the number of available root hub ports. */
static uint8_t uhci_detect_ports(uhci_state_t *uhci) {
  uint8_t port = 0;

  for (; uhci_is_port(uhci, port); port++) {
    if (!uhci_check_port(uhci, port))
      return 0;
  }

  uhci->nports = port;
  klog("detected %hhu ports", uhci->nports);

  return port;
}

/* Return the number of available root hub ports. */
static uint8_t uhci_number_of_ports(device_t *dev) {
  uhci_state_t *uhci = dev->state;
  return uhci->nports;
}

/* Check whether the device attached to the specified port
 * is a low speed device. */
static bool uhci_low_speed(uhci_state_t *uhci, uint8_t port) {
  return chkw(UHCI_PORTSC(port), UHCI_PORTSC_LSDA);
}

static int print_usb_release(device_t *dev) {
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

  /* Is it a USB compatible controller? */
  if (pcid->class_code != PCI_USB_CLASSCODE ||
      pcid->subclass_code != PCI_USB_SUBCLASSCODE)
    return 0;

  /* Is it a UHCI controller? */
  if (pcid->progif != PCI_INTERFACE_UHCI)
    return 0;

  if (print_usb_release(dev))
    return 0;

  return 1;
}

static int uhci_attach(device_t *dev) {
  uhci_state_t *uhci = dev->state;

  /* Gather required resources. */
  uhci->regs = device_take_ioports(dev, 4, RF_ACTIVE);
  uhci->irq = device_take_irq(dev, 0, RF_ACTIVE);
  assert(uhci->regs);
  assert(uhci->irq);

  bus_intr_setup(dev, uhci->irq, uhci_isr, NULL, uhci, "UHCI");

  /* Perform the global reset of the UHCI controller. */
  setw(UHCI_CMD, UHCI_CMD_GRESET);
  mdelay(10);
  setw(UHCI_CMD, 0x0000);

  /* After global reset, all registers are set to their
   * default values. */
  if (inw(UHCI_CMD) != 0x0000)
    return ENXIO;
  if (inw(UHCI_STS) != UHCI_STS_HCH)
    return ENXIO;
  if (inb(UHCI_SOF) != 0x40) /* each frame is 1 ms */
    return ENXIO;

  /* Unhalt the controller. */
  wclrw(UHCI_STS, UHCI_STS_HCH);

  /* Perform the host controller reset. */
  setw(UHCI_CMD, UHCI_CMD_HCRESET);
  while (inw(UHCI_CMD) & UHCI_CMD_HCRESET)
    ;

  if (!uhci_detect_ports(uhci))
    return ENXIO;

  /* Enable bus master mode. */
  pci_enable_busmaster(dev);

  /* Initialize the software context. */
  uhci_init_mainqs(uhci);
  uhci_init_frames(uhci);
  uhci_init_pool();

  /* Set current frame number to the first frame pointer. */
  outw(UHCI_FRNUM, 0x0000);

  /* Turn on the IOC and error interrupts. */
  setw(UHCI_INTR, UHCI_INTR_TOCRCIE | UHCI_INTR_IOCE);

  /* Start the controller. */
  outw(UHCI_CMD, UHCI_CMD_RS);

  /* Initialize the underlying USB bus. */
  usb_init(dev);

  /* Detect and configure attached devices. */
  return usb_enumerate(dev);
}

static usbhc_methods_t uhci_usbhc_if = {
  .number_of_ports = uhci_number_of_ports,
  .device_present = uhci_device_present,
  .reset_port = uhci_reset_port,
  .control_transfer = uhci_control_transfer,
  .interrupt_transfer = uhci_interrupt_transfer,
  .bulk_transfer = uhci_bulk_transfer,
};

static driver_t uhci = {
  .desc = "UHCI driver",
  .size = sizeof(uhci_state_t),
  .pass = SECOND_PASS,
  .probe = uhci_probe,
  .attach = uhci_attach,
  .interfaces =
    {
      [DIF_USBHC] = &uhci_usbhc_if,
    },
};

DEVCLASS_ENTRY(pci, uhci);
