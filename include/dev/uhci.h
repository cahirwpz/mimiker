/*
 * Based on FreeBSD `sys/dev/usb/controller/uhci.h`.
 */
#ifndef _DEV_UHCI_H_
#define _DEV_UHCI_H_

#include <sys/spinlock.h>

#define UHCI_FRAMELIST_COUNT 1024 /* units */

/* Structures alignment (bytes) */
#define UHCI_TD_ALIGN 16
#define UHCI_QH_ALIGN 16

typedef uint32_t uhci_physaddr_t;

#define UHCI_PTR_T 0x00000001
#define UHCI_PTR_TD 0x00000000
#define UHCI_PTR_QH 0x00000002
#define UHCI_PTR_VF 0x00000004
#define UHCI_PTR_MASK 0x00000007

typedef struct uhci_td uhci_td_t;
typedef struct uhci_qh uhci_qh_t;
typedef TAILQ_HEAD(qh_list, uhci_qh) uhci_qh_list_t;

struct uhci_td {
  volatile uint32_t td_next;
  volatile uint32_t td_status;
  volatile uint32_t td_token;
  volatile uint32_t td_buffer;
  uint32_t __reserved[4]; /* 16 bytes for user usage */
} __aligned(UHCI_TD_ALIGN);

#define UHCI_TD_GET_ACTLEN(s) (((s) + 1) & 0x3ff)
#define UHCI_TD_BITSTUFF 0x00020000
#define UHCI_TD_CRCTO 0x00040000
#define UHCI_TD_NAK 0x00080000
#define UHCI_TD_BABBLE 0x00100000
#define UHCI_TD_DBUFFER 0x00200000
#define UHCI_TD_STALLED 0x00400000
#define UHCI_TD_ACTIVE 0x00800000
#define UHCI_TD_IOC 0x01000000
#define UHCI_TD_IOS 0x02000000
#define UHCI_TD_LS 0x04000000
#define UHCI_TD_GET_ERRCNT(s) (((uint32_t)(s) >> 27) & 3)
#define UHCI_TD_SET_ERRCNT(n) ((n) << 27)
#define UHCI_TD_SPD 0x20000000
#define UHCI_TD_PID 0x000000ff
#define UHCI_TD_PID_IN 0x00000069
#define UHCI_TD_PID_OUT 0x000000e1
#define UHCI_TD_PID_SETUP 0x0000002d
#define UHCI_TD_GET_PID(s) ((s)&0xff)
#define UHCI_TD_SET_DEVADDR(a) ((uint32_t)(a) << 8)
#define UHCI_TD_GET_DEVADDR(s) (((uint32_t)(s) >> 8) & 0x7f)
#define UHCI_TD_SET_ENDPT(e) (((uint32_t)(e)&0xf) << 15)
#define UHCI_TD_GET_ENDPT(s) (((uint32_t)(s) >> 15) & 0xf)
#define UHCI_TD_SET_DT(t) ((uint32_t)(t) << 19)
#define UHCI_TD_GET_DT(s) (((uint32_t)(s) >> 19) & 1)
#define UHCI_TD_SET_MAXLEN(l) (((l)-1U) << 21)
#define UHCI_TD_GET_MAXLEN(s) ((((uint32_t)(s) >> 21) + 1) & 0x7ff)
#define UHCI_TD_MAXLEN_MASK 0xffe00000
#define UHCI_TD_MAXLEN (1 << 11)

#define UHCI_TD_ERROR                                                          \
  (UHCI_TD_BITSTUFF | UHCI_TD_CRCTO | UHCI_TD_BABBLE | UHCI_TD_DBUFFER |       \
   UHCI_TD_STALLED)

#define UHCI_TD_SETUP(len, endpt, dev)                                         \
  (UHCI_TD_SET_MAXLEN(len) | UHCI_TD_SET_ENDPT(endpt) |                        \
   UHCI_TD_SET_DEVADDR(dev) | UHCI_TD_PID_SETUP)

#define UHCI_TD_OUT(len, endpt, dev, dt)                                       \
  (UHCI_TD_SET_MAXLEN(len) | UHCI_TD_SET_ENDPT(endpt) |                        \
   UHCI_TD_SET_DEVADDR(dev) | UHCI_TD_PID_OUT | UHCI_TD_SET_DT(dt))

#define UHCI_TD_IN(len, endpt, dev, dt)                                        \
  (UHCI_TD_SET_MAXLEN(len) | UHCI_TD_SET_ENDPT(endpt) |                        \
   UHCI_TD_SET_DEVADDR(dev) | UHCI_TD_PID_IN | UHCI_TD_SET_DT(dt))

typedef struct usb_buf usb_buf_t;

struct uhci_qh {
  volatile uint32_t qh_h_next;
  volatile uint32_t qh_e_next;
  union {
    /* Main queue items. */
    struct {
      spin_t qh_lock;         /* guards all the main queue's fields */
      uhci_qh_list_t qh_list; /* list of scheduled transfers */
    };
    /* External queue items. */
    struct {
      TAILQ_ENTRY(uhci_qh) qh_link; /* link on a main queue's list */
      usb_buf_t *qh_buf; /* usb buffer associated with the transaction */
      void *qh_data;     /* I/O data buffer of the transaction */
    };
  };
} __aligned(UHCI_QH_ALIGN);

#endif /* _DEV_UHCI_H_ */
