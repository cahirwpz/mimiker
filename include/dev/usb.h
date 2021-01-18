#ifndef _USB_H_
#define _USB_H_

#include <sys/ringbuf.h>
#include <sys/condvar.h>
#include <sys/spinlock.h>

struct usb_dev_req {
  uint8_t type;
  uint8_t req;
  uint16_t val;
  uint16_t idx;
  uint16_t len;
} __packed;

typedef struct usb_dev_req usb_dev_req_t;

#define UT_WRITE 0x00
#define UT_READ 0x80
#define UT_STANDARD 0x00
#define UT_CLASS 0x20
#define UT_VENDOR 0x40
#define UT_DEVICE 0x00
#define UT_INTERFACE 0x01
#define UT_ENDPOINT 0x02
#define UT_OTHER 0x03

#define UT_READ_INTERFACE (UT_READ | UT_STANDARD | UT_INTERFACE)
#define UT_READ_ENDPOINT (UT_READ | UT_STANDARD | UT_ENDPOINT)
#define UT_WRITE_ENDPOINT (UT_WRITE | UT_STANDARD | UT_ENDPOINT)
#define UT_READ_CLASS_INTERFACE (UT_READ | UT_CLASS | UT_INTERFACE)
#define UT_READ_CLASS_ENDPOINT (UT_READ | UT_CLASS | UT_ENDPOINT)
#define UT_WRITE_CLASS_INTERFACE (UT_WRITE | UT_CLASS | UT_INTERFACE)

/* Requests. */
#define UR_GET_STATUS 0x00
#define UR_CLEAR_FEATURE 0x01
#define UR_SET_ADDRESS 0x05
#define UR_GET_DESCRIPTOR 0x06
#define USB_LANGUAGE_TABLE 0x00
#define UR_GET_CONFIG 0x08
#define UR_SET_CONFIG 0x09
#define UDESC_DEVICE 0x01
#define UDESC_CONFIG 0x02
#define UDESC_STRING 0x03

/* Feature numbers. */
#define UF_ENDPOINT_HALT 0

#define UV_MAKE(d, i) ((d) << 8 | (i))

struct usb_dev_dsc {
  uint8_t len;
  uint8_t type;
  uint16_t usb_release;
  uint8_t dev_class;
  uint8_t dev_subclass;
  uint8_t dev_protocol;
  uint8_t max_pkt_size;
  uint16_t vendor_id;
  uint16_t product_id;
  uint16_t dev_release;
  uint8_t manufacturer_idx;
  uint8_t product_idx;
  uint8_t serialnum_idx;
  uint8_t nconfigurations;
} __packed;

typedef struct usb_dev_dsc usb_dev_dsc_t;

#define SDSC_DATASIZE 126

struct usb_str_dsc {
  uint8_t len;
  uint8_t type;
  uint16_t string[SDSC_DATASIZE];
  uint8_t unused;
} __packed;

typedef struct usb_str_dsc usb_str_dsc_t;

struct usb_str_lang {
  uint8_t len;
  uint8_t type;
  uint16_t data[SDSC_DATASIZE];
} __packed;

typedef struct usb_str_lang usb_str_lang_t;

#define USENG_LID 0x0409
#define USENG_STR "English (United States)"

struct usb_config_dsc {
  uint8_t len;
  uint8_t type;
  uint16_t total_len;
  uint8_t ninterfaces;
  uint8_t config_val;
  uint8_t config_idx;
  uint8_t attributes;
  uint8_t max_power; /* max current in 2 mA units */
} __packed;

typedef struct usb_config_dsc usb_config_dsc_t;

struct usb_interface_dsc {
  uint8_t len;
  uint8_t type;
  uint8_t inum;
  uint8_t alt_setting;
  uint8_t nendpoints;
  uint8_t iclass;
  uint8_t isubclass;
  uint8_t iprotocol;
  uint8_t iidx;
} __packed;

typedef struct usb_interface_dsc usb_interface_dsc_t;

#define UICLASS_HID 0x03
#define UISUBCLASS_BOOT 1
#define UIPROTO_BOOT_KEYBOARD 1
#define UIPROTO_MOUSE 2

#define UICLASS_MASS 0x08
#define UISUBCLASS_SCSI 6
#define UIPROTO_MASS_BBB 80

struct usb_endp_dsc {
  uint8_t len;
  uint8_t type;
  uint8_t addr;
  uint8_t attributes;
  uint16_t max_pkt_size;
  uint8_t interval;
} __packed;

typedef struct usb_endp_dsc usb_endp_dsc_t;

#define UE_TRANSFER_TYPE(at) ((at)&0x03)

#define UE_CONTROL 0x00
#define UE_ISOCHRONOUS 0x01
#define UE_BULK 0x02
#define UE_INTERRUPT 0x03

#define UE_ADDR(ad) ((ad)&0x0f)
#define UE_DIR(ad) ((ad)&0x80)

typedef struct usb_device {
  usb_dev_dsc_t dd;
  usb_endp_dsc_t *endps; /* endpoints supplied by interface `inum` */
  uint8_t nendps;        /* numer of endpoints */
  uint8_t addr;          /* device address */
  uint8_t inum;          /* interface number */
  uint8_t port;          /* roothub port number */
} usb_device_t;

typedef enum {
  TF_INPUT = 1,
  TF_CONTROL = 2,
  TF_BULK = 4,
  TF_INTERRUPT = 8,
  TF_STALLED = 16,
  TF_ERROR = 32, /* errors other than STALL */
} transfer_flags_t;

typedef struct usb_buf {
  ringbuf_t buf;
  condvar_t cv;
  spin_t lock;
  transfer_flags_t flags;
  uint16_t req_len;
} usb_buf_t;

static inline usb_device_t *usb_device_of(device_t *device) {
  return (usb_device_t *)device->instance;
}

static inline int usb_endp_type(usb_device_t *usbd, uint8_t idx) {
  assert(idx < usbd->nendps);
  return UE_TRANSFER_TYPE(usbd->endps[idx].attributes);
}

static inline int usb_endp_dir(usb_device_t *usbd, uint8_t idx) {
  assert(idx < usbd->nendps);
  return UE_DIR(usbd->endps[idx].addr);
}

uint16_t usb_max_pkt_size(usb_device_t *usbd, usb_buf_t *usbb);
uint8_t usb_endp_addr(usb_device_t *usbd, usb_buf_t *usbb);
uint8_t usb_status_type(usb_buf_t *usbb);
uint8_t usb_interval(usb_device_t *usbd, usb_buf_t *usbb);
void usb_process(usb_buf_t *usbb, void *data, transfer_flags_t flags);
usb_buf_t *usb_alloc_buf(void *data, size_t size, transfer_flags_t flags,
                         uint16_t req_len);

#define usb_alloc_complete_buf(data, size, flags)                              \
  usb_alloc_buf((data), (size), (flags), (size))

void usb_free_buf(usb_buf_t *usbb);
void usb_reuse_buf(usb_buf_t *usbb, void *data, size_t size,
                   transfer_flags_t flags, uint16_t req_len);

#define usb_reuse_complete_buf(usbb, data, size, flags)                        \
  usb_reuse_buf((usbb), (data), (size), (flags), (size))

void usb_reset_buf(usb_buf_t *usbb, uint16_t req_len);

#define usb_reset_complete_buf(usbb) usb_reset_buf((usbb), (usbb)->req_len)

void usb_wait(usb_buf_t *usbb);
bool usb_transfer_error(usb_buf_t *usbb);
int usb_unhalt_endp(usb_device_t *usbd, uint8_t idx);
int usb_set_idle(usb_device_t *usbd);
int usb_set_boot_protocol(usb_device_t *usbd);
int usb_get_report(usb_device_t *usbd, void *buf, size_t size);
int usb_get_max_lun(usb_device_t *usbd, uint8_t *maxlun);
int usb_bulk_only_reset(usb_device_t *usbd);
void usb_interrupt_transfer(usb_device_t *usbd, usb_buf_t *usbb);
int usb_poll(usb_device_t *usbd, usb_buf_t *usbb, uint8_t idx, void *buf,
             size_t size);
void usb_bulk_transfer(usb_device_t *usbd, usb_buf_t *usbb);
void usb_enumerate(device_t *dev);

#endif /* _USB_H_ */
