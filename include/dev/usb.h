/*
 * Based on FreeBSD `usb.h`.
 */
#ifndef _USB_H_
#define _USB_H_

#include <sys/ringbuf.h>
#include <sys/condvar.h>
#include <sys/spinlock.h>

typedef struct usb_device_request {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} __packed usb_device_request_t;

#define UT_WRITE 0x00
#define UT_READ 0x80
#define UT_STANDARD 0x00
#define UT_CLASS 0x20
#define UT_DEVICE 0x00
#define UT_INTERFACE 0x01
#define UT_ENDPOINT 0x02

#define UT_READ_DEVICE (UT_READ | UT_STANDARD | UT_DEVICE)
#define UT_READ_INTERFACE (UT_READ | UT_STANDARD | UT_INTERFACE)
#define UT_READ_ENDPOINT (UT_READ | UT_STANDARD | UT_ENDPOINT)
#define UT_WRITE_DEVICE (UT_WRITE | UT_STANDARD | UT_DEVICE)
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

typedef struct usb_device_descriptor {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize;
  /* The fields below are not part of the initial descriptor. */
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
} __packed usb_device_descriptor_t;

#define US_DATASIZE 126

typedef struct usb_string_descriptor {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bString[US_DATASIZE];
  uint8_t bUnused;
} __packed usb_string_descriptor_t;

typedef struct usb_string_lang {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bData[US_DATASIZE];
} __packed usb_string_lang_t;

#define US_ENG_LID 0x0409
#define US_ENG_STR "English (United States)"

typedef struct usb_config_descriptor {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterface;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower; /* max current in 2 mA units */
} __packed usb_config_descriptor_t;

typedef struct usb_interface_descriptor {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
} __packed usb_interface_descriptor_t;

/* Interface class codes */
#define UICLASS_HID 0x03
#define UISUBCLASS_BOOT 1
#define UIPROTO_BOOT_KEYBOARD 1
#define UIPROTO_MOUSE 2

#define UICLASS_MASS 0x08
#define UISUBCLASS_SCSI 6
#define UIPROTO_MASS_BBB 80

typedef struct usb_endpoint_descriptor {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
} __packed usb_endpoint_descriptor_t;

#define UE_ADDR 0x0f
#define UE_DIR_IN 0x80  /* IN-token endpoint */
#define UE_DIR_OUT 0x00 /* OUT-token endpoint */
#define UE_GET_DIR(a) ((a)&0x80)
#define UE_GET_ADDR(a) ((a)&UE_ADDR)

#define UE_TRANSFER_TYPE(at) ((at)&0x03)

#define UE_CONTROL 0x00
#define UE_ISOCHRONOUS 0x01
#define UE_BULK 0x02
#define UE_INTERRUPT 0x03

typedef struct usb_device {
  usb_device_descriptor_t dd;
  usb_endpoint_descriptor_t *endps; /* endpoints supplied by interface `inum` */
  uint8_t nendps;                   /* numer of endpoints */
  uint8_t addr;                     /* device address */
  uint8_t inum;                     /* interface number */
  uint8_t port;                     /* roothub port number */
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
  uint16_t transfer_size;
} usb_buf_t;

static inline usb_device_t *usb_device_of(device_t *device) {
  return (usb_device_t *)device->instance;
}

static inline int usb_endp_type(usb_device_t *usbd, uint8_t idx) {
  assert(idx < usbd->nendps);
  return UE_TRANSFER_TYPE(usbd->endps[idx].bmAttributes);
}

static inline int usb_endp_dir(usb_device_t *usbd, uint8_t idx) {
  assert(idx < usbd->nendps);
  return UE_GET_DIR(usbd->endps[idx].bEndpointAddress);
}

uint16_t usb_max_pkt_size(usb_device_t *usbd, usb_buf_t *usbb);
uint8_t usb_endp_addr(usb_device_t *usbd, usb_buf_t *usbb);
uint8_t usb_status_type(usb_buf_t *usbb);
uint8_t usb_interval(usb_device_t *usbd, usb_buf_t *usbb);
void usb_process(usb_buf_t *usbb, void *data, transfer_flags_t flags);
usb_buf_t *usb_alloc_buf(void *data, size_t size, transfer_flags_t flags,
                         uint16_t transfer_size);

#define usb_alloc_buf_from_struct(sp, flags, ts)                               \
  usb_alloc_buf((sp), sizeof(*(sp)), (flags), (ts))

#define usb_alloc_empty_buf(flags) usb_alloc_buf(NULL, 0, (flags), 0)

#define usb_alloc_bulk_buf(data, size, flags)                                  \
  usb_alloc_buf((data), (size), (flags), (size))

void usb_free_buf(usb_buf_t *usbb);
void usb_reuse_buf(usb_buf_t *usbb, void *data, size_t size,
                   transfer_flags_t flags, uint16_t transfer_size);

#define usb_reuse_bulk_buf(usbb, data, size, flags)                            \
  usb_reuse_buf((usbb), (data), (size), (flags), (size))

void usb_reset_buf(usb_buf_t *usbb, uint16_t transfer_size);

#define usb_reset_bulk_buf(usbb) usb_reset_buf((usbb), (usbb)->buf.size)

void usb_wait(usb_buf_t *usbb);
bool usb_transfer_error(usb_buf_t *usbb);
int usb_unhalt_endp(usb_device_t *usbd, uint8_t idx);
int usb_set_idle(usb_device_t *usbd);
int usb_set_boot_protocol(usb_device_t *usbd);
int usb_bbb_get_max_lun(usb_device_t *usbd, uint8_t *maxlun);
int usb_bbb_reset(usb_device_t *usbd);
void usb_interrupt_transfer(usb_device_t *usbd, usb_buf_t *usbb);
int usb_poll(usb_device_t *usbd, usb_buf_t *usbb, uint8_t idx, void *buf,
             size_t size);
void usb_bulk_transfer(usb_device_t *usbd, usb_buf_t *usbb);
void usb_enumerate(device_t *dev);

#endif /* _USB_H_ */
