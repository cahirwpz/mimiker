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

/*
 * USB device request.
 */
typedef struct usb_dev_req {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} __packed usb_dev_req_t;

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

/*
 * USB device descriptor.
 */
typedef struct usb_dev_dsc {
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
} __packed usb_dev_dsc_t;

#define US_DATASIZE 63

/*
 * USB string descriptor.
 */
typedef struct usb_str_dsc {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bString[US_DATASIZE];
  uint8_t bUnused;
} __packed usb_str_dsc_t;

/*
 * USB string language descriptor.
 */
typedef struct usb_str_lang {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bData[US_DATASIZE];
} __packed usb_str_lang_t;

#define US_ENG_LID 0x0409
#define US_ENG_STR "English (United States)"

/*
 * USB configuration descriptor.
 */
typedef struct usb_config_dsc {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterface;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower; /* max current in 2 mA units */
} __packed usb_config_dsc_t;

/*
 * USB interface descriptor.
 */
typedef struct usb_if_dsc {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
} __packed usb_if_dsc_t;

/* Interface class codes */
#define UICLASS_HID 0x03
#define UISUBCLASS_BOOT 1
#define UIPROTO_BOOT_KEYBOARD 1
#define UIPROTO_MOUSE 2

#define UICLASS_MASS 0x08
#define UISUBCLASS_SCSI 6
#define UIPROTO_MASS_BBB 80

/*
 * USB endpoint descriptor.
 */
typedef struct usb_endp_dsc {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
} __packed usb_endp_dsc_t;

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
} __packed usb_error_t;

/* Don't alter the following values! */
typedef enum usb_transfer {
  USB_TFR_NONE = 0,
  USB_TFR_CONTROL = 1,
  USB_TFR_ISOCHRONOUS = 2,
  USB_TFR_BULK = 3,
  USB_TFR_INTERRUPT = 4,
} __packed usb_transfer_t;

typedef enum usb_direction {
  USB_DIR_INPUT,
  USB_DIR_OUTPUT,
} __packed usb_direction_t;

typedef struct usb_endp {
  TAILQ_ENTRY(usb_endp) link; /* entry on device's endpoint list */
  uint16_t maxpkt;            /* max packet size */
  uint8_t addr;               /* address within a device */
  usb_transfer_t transfer;    /* transfer type */
  usb_direction_t dir;        /* transfer direction */
  uint8_t interval;           /* interval for polling data transfers */
} usb_endp_t;

/* USB device software representation. */
typedef struct usb_device {
  TAILQ_HEAD(, usb_endp) endps; /* endpoints provided by the device */
  uint8_t addr;                 /* address of the device */
  uint8_t port;                 /* root hub port number */
  uint8_t ifnum;                /* current interface number */
  uint8_t class_code;           /* device class code */
  uint8_t subclass_code;        /* device subclass code */
  uint8_t protocol_code;        /* protocol code */
  uint16_t vendor_id;           /* vendor ID */
  uint16_t product_id;          /* product ID */
} usb_device_t;

typedef struct usb_buf usb_buf_t;

static inline usb_device_t *usb_device_of(device_t *dev) {
  return dev->bus == DEV_BUS_USB ? dev->instance : NULL;
}

static inline device_t *usb_bus_of(device_t *dev) {
  return dev->bus == DEV_BUS_USB ? dev->parent : TAILQ_FIRST(&dev->children);
}

/* Copies data bytes form `src` to buffer `buf`'s internal buffer.
 * Before copying, the internal buffer is reseted. This function should
 * be used before performing output transfers on a buffer. */
void usb_buf_copyin(usb_buf_t *buf, void *src, size_t size);

/* Copies data bytes contained in `buf` to designated area `dst`.
 * Data is copied from the beginning of an internal buffer and isn't mark
 * as readed. For reading data form a buffer use `usb_buf_read` instead. */
void usb_buf_copyout(usb_buf_t *buf, void *dst, size_t size);

/* Allocate a USB buffer with internal buffer of size `size`. */
usb_buf_t *usb_buf_alloc(size_t size);

/* Releases a previously allocated buffer. */
void usb_buf_free(usb_buf_t *buf);

/* Returns current `buf`'s transfer direction. */
usb_direction_t usb_buf_dir(usb_buf_t *buf);

/* Returns current `buf`'s transfer data stage size. */
uint16_t usb_buf_transfer_size(usb_buf_t *buf);

/* Processes data `data` received in transfer in which `buf` is used,
 * or processes error `error` encountered during the transfer. */
void usb_buf_process(usb_buf_t *buf, void *data, usb_error_t error);

/* Returns true if the transfer in which `buf` is being used in is peridoc. */
bool usb_buf_periodic(usb_buf_t *buf);

/* Returns `buf`'s error status. The error status is always cleared upon
 * passing `buf` to any USB transfer function (as that means the buffer is
 * beaing reused). */
usb_error_t usb_buf_error(usb_buf_t *buf);

/* Reads bytes received in lates `buf`'s transfer, or waits for the data
 * to arrive. The only supported value of `flags` is `IO_NONBLOCK`.
 * If data isn't ready yet, and `IO_NONBLOCK` is specified, the function
 * immediately returns with `EAGAIN`. If an error other than `EAGAIN`
 * is returned, further information may be obtained through `usb_buf_error`. */
int usb_buf_read(usb_buf_t *buf, void *dst, int flags);

/* Waits for the data involved in `buf`'s latest transfer to be written
 * to the destioation device. The only supported value of `flags`
 * is `IO_NONBLOCK`. If data isn't already written, and `IO_NONBLOCK`
 * is specified, the function immediately returns with `EAGAIN`.
 * If an error other than `EAGAIN` is returned, further information
 * may be obtained through `usb_buf_error`. */
int usb_buf_write(usb_buf_t *buf, int flags);

/* Returns direction for the STATUS stage of a transfer.
 * Should only be used by USB host controller drivers. */
usb_direction_t usb_status_dir(usb_direction_t dir, uint16_t transfer_size);

/* Initializes the underlying USB bus of the host controller `hcdev`. */
void usb_init(device_t *hcdev);

/* Enumerates and configures all devices attached to root hub `hcdev`. */
int usb_enumerate(device_t *hcdev);

/*
 * USB standard interface.
 *
 * The following interface provides basic USB transfers: control, interrupt,
 * and bulk. Although control transfers are supplied, if there is a need to
 * perform some standard request form a device driver, it should be added to
 * USB bus standard requests interface.
 */

typedef void (*usb_control_transfer_t)(device_t *dev, usb_buf_t *buf,
                                       usb_direction_t dir, usb_dev_req_t *req);
typedef void (*usb_interrupt_transfer_t)(device_t *dev, usb_buf_t *buf,
                                         usb_direction_t dir, uint16_t size);
typedef void (*usb_bulk_transfer_t)(device_t *dev, usb_buf_t *buf,
                                    usb_direction_t dir, uint16_t size);

typedef struct usb_methods {
  usb_control_transfer_t control_transfer;
  usb_interrupt_transfer_t interrupt_transfer;
  usb_bulk_transfer_t bulk_transfer;
} usb_methods_t;

static inline usb_methods_t *usb_methods(device_t *dev) {
  return (usb_methods_t *)dev->driver->interfaces[DIF_USB];
}

/* Issues a control transfer of direction `dir`, with device request `req`
 * using buffer `buf`. This is an asynchronous function. In order to receive
 * involved data use `usb_buf_read`. To wait for the data to be written
 * use `usb_buf_write`. All encountered errors will be reflected in `buf`'s
 * error status (see `usb_buf_error`). */
static inline void usb_control_transfer(device_t *dev, usb_buf_t *buf,
                                        usb_direction_t dir,
                                        usb_dev_req_t *req) {
  usb_methods(dev->parent)->control_transfer(dev, buf, dir, req);
}

/* Issues an interrupt transfer of direction `dir`, using buffer `buf`.
 * This is an asynchronous function. In order to receive involved data use
 * `usb_buf_read`. To wait for the data to be written use `usb_buf_write`.
 * All encountered errors will be reflected in `buf`'s error status
 * (see `usb_buf_error`). Input interrupt transfers are periodic, thus
 * `usb_buf_read` can be called multiple times. */
static inline void usb_interrupt_transfer(device_t *dev, usb_buf_t *buf,
                                          usb_direction_t dir, size_t size) {
  usb_methods(dev->parent)->interrupt_transfer(dev, buf, dir, size);
}

/* Issues a bulk transfer of direction `dir`, using buffer `buf`.
 * This is an asynchronous function. In order to receive involved data use
 * `usb_buf_read`. To wait for the data to be written use `usb_buf_write`.
 * All encountered errors will be reflected in `buf`'s  error status
 * (see `usb_buf_error`). */
static inline void usb_bulk_transfer(device_t *dev, usb_buf_t *buf,
                                     usb_direction_t dir, size_t size) {
  usb_methods(dev->parent)->bulk_transfer(dev, buf, dir, size);
}

/*
 * USB standard requests interface.
 *
 * The following interface provides standard USB requests. Requests used for
 * device identification and configuration aren't supplied since USB bus
 * driver identifies and configures each device automatically during enumeration
 * process. It is suggested to add a new method to this interface instead of
 * using `usb_control_transfer` request if a need occurs.
 */

typedef int (*usb_unhalt_endp_t)(device_t *dev, usb_transfer_t transfer,
                                 usb_direction_t dir);

typedef struct usb_req_methods {
  usb_unhalt_endp_t unhalt_endp;
} usb_req_methods_t;

static inline usb_req_methods_t *usb_req_methods(device_t *dev) {
  return (usb_req_methods_t *)dev->driver->interfaces[DIF_USB_REQ];
}

/* Unhalts endpoint of device `dev` which implements transfer type `transfer`
 * with direction `dir`. */
static inline int usb_unhalt_endp(device_t *dev, usb_transfer_t transfer,
                                  usb_direction_t dir) {
  return usb_req_methods(dev->parent)->unhalt_endp(dev, transfer, dir);
}

/*
 * USB HID specific standard requests interface.
 *
 * The following interface provides standard USB request specific to
 * HID device class.
 */

typedef int (*usb_hid_set_idle_t)(device_t *dev);
typedef int (*usb_hid_set_boot_protocol_t)(device_t *dev);

typedef struct usb_hid_methods {
  usb_hid_set_idle_t set_idle;
  usb_hid_set_boot_protocol_t set_boot_protocol;
} usb_hid_methods_t;

static inline usb_hid_methods_t *usb_hid_methods(device_t *dev) {
  return (usb_hid_methods_t *)dev->driver->interfaces[DIF_USB_HID];
}

/* Tells device `dev` to inhibit all reports until a report changes. */
static inline int usb_hid_set_idle(device_t *dev) {
  return usb_hid_methods(dev->parent)->set_idle(dev);
}

/* Sets `dev`'s report format to the boot interface report format. */
static inline int usb_hid_set_boot_protocol(device_t *dev) {
  return usb_hid_methods(dev->parent)->set_boot_protocol(dev);
}

/*
 * USB Bulk-Only specific standard requests interface.
 *
 * The following interface provides standard USB request specific to
 * devices which rely on the Bulk-Only protocol.
 * BBB refers Bulk/Bulk/Bulk for Command/Data/Status phases.
 */

typedef int (*usb_bbb_get_max_lun_t)(device_t *dev, uint8_t *maxlun);
typedef int (*usb_bbb_reset_t)(device_t *dev);

typedef struct usb_bbb_methods {
  usb_bbb_get_max_lun_t get_max_lun;
  usb_bbb_reset_t reset;
} usb_bbb_methods_t;

static inline usb_bbb_methods_t *usb_bbb_methods(device_t *dev) {
  return (usb_bbb_methods_t *)dev->driver->interfaces[DIF_USB_BBB];
}

/* Retrives the maximum Logical Unit Number of device `dev`. */
static inline int usb_bbb_get_max_lun(device_t *dev, uint8_t *maxlun) {
  return usb_bbb_methods(dev->parent)->get_max_lun(dev, maxlun);
}

/* Resets mass storage device `dev`. */
static inline int usb_bbb_reset(device_t *dev) {
  return usb_bbb_methods(dev->parent)->reset(dev);
}

#endif /* _DEV_USB_H_ */
