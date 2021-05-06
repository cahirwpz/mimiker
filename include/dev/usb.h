/*
 * Based on FreeBSD `sys/dev/usb/usb.h`.
 */
#ifndef _DEV_USB_H_
#define _DEV_USB_H_

#include <sys/ringbuf.h>
#include <sys/condvar.h>
#include <sys/spinlock.h>
#include <sys/device.h>

/*
 * Constructs defined by the USB specification.
 */

/* Definition of some hardcoded USB constants. */

#define USB_MAX_IPACKET 8 /* initial USB packet size */

/* These are the values from the USB specification. */
#define USB_PORT_ROOT_RESET_DELAY_SPEC 50 /* ms */
#define USB_PORT_RESET_RECOVERY_SPEC 10   /* ms */

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

#define US_DATASIZE 63

typedef struct usb_string_descriptor {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bString[US_DATASIZE];
  uint8_t bUnused;
} __packed usb_string_descriptor_t;

typedef struct usb_string_lang {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bData[US_DATASIZE];
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

/*
 * Implementation specific constructs.
 */

typedef enum usb_error {
  USB_ERR_STALLED = 1, /* STALL condition encountered */
  USB_ERR_OTHER = 2,   /* errors other than STALL */
} usb_error_t;

typedef enum usb_transfer {
  USB_TFR_NONE,
  USB_TFR_CONTROL,
  USB_TFR_INTERRUPT,
  USB_TFR_BULK,
} usb_transfer_t;

typedef enum usb_direction {
  USB_DIR_INPUT,
  USB_DIR_OUTPUT,
} usb_direction_t;

/* USB buffer used for USB transfers. */
typedef struct usb_buf {
  ringbuf_t rb;            /* write source or read destination */
  condvar_t cv;            /* wait for the transfer to complete */
  spin_t lock;             /* buffer guard */
  usb_error_t error;       /* errors encountered during transfer */
  usb_transfer_t transfer; /* what kind of transfer is this ? */
  usb_direction_t dir;     /* transfer direction */
  uint16_t transfer_size;  /* size of the data stage */
} usb_buf_t;

typedef struct usb_device usb_device_t;

static inline usb_device_t *usb_device_of(device_t *dev) {
  return dev->bus == DEV_BUS_USB ? dev->instance : NULL;
}

static inline device_t *usb_bus_of(device_t *dev) {
  return dev->bus == DEV_BUS_USB ? dev->parent : TAILQ_FIRST(&dev->children);
}

/* Returns true if the transfer described by `buf` is peridic. */
bool usb_buf_periodic(usb_buf_t *buf);

/* Returns direction for the STATUS stage of a transfer. */
usb_direction_t usb_buf_status_direction(usb_buf_t *buf);

/* Copy data bytes contained in `buf` to designated area `dst`. */
void usb_buf_copy_data(usb_buf_t *buf, void *dst);

/* Initialize the underlying USB bus of the host controller `dev`. */
void usb_init(device_t *dev);

/* Enumerate and configure all devices attached to root hub `dev`.
 * Returns an error code. */
int usb_enumerate(device_t *dev);

/* Process data `data` received in the transfer described by `buf`,
 * or process error `error` encountered during the transfer. */
void usb_process(usb_buf_t *buf, void *data, usb_error_t error);

#endif /* _DEV_USB_H_ */
