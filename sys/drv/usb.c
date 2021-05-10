/*
 * USB bus driver.
 *
 * For explanation of terms used throughout the code
 * please see the following documents:
 * - USB 2.0 specification:
 *     http://sdpha2.ucsd.edu/Lab_Equip_Manuals/usb_20.pdf
 *
 * Each inner function if given a description. For description
 * of the rest of contained functions please see `include/dev/usb.h`.
 * Each function which composes an interface is given a leading underscore
 * in orded to avoid symbol confilcts with interface wrappers presented in
 * aforementioned header file.
 */
#define KL_LOG KL_DEV
#include <sys/errno.h>
#include <sys/devclass.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/klog.h>
#include <sys/file.h>
#include <sys/bus.h>
#include <dev/usb.h>
#include <dev/usbhc.h>
#include <dev/usbhid.h>
#include <dev/umass.h>

/* USB bus software state. */
typedef struct usb_state {
  uint8_t next_addr; /* next device address to grant */
} usb_state_t;

/* Indexes in USB device's index table which is used to save device's
 * string descriptor intexes. */
typedef enum usb_idx {
  USB_IDX_MANUFACTURER = 0,
  USB_IDX_PRODUCT = 1,
  USB_IDX_SERIAL_NUMBER = 2,
  USB_IDX_CONFIGURATION = 3,
  USB_IDX_INTERFACE = 4,
  USB_IDX_COUNT
} usb_idx_t;

/* Messages used in printing string descriptors. */
static const char *idx_info[] = {
  [USB_IDX_MANUFACTURER] = "manufacturer",
  [USB_IDX_PRODUCT] = "product",
  [USB_IDX_SERIAL_NUMBER] = "serial number",
  [USB_IDX_CONFIGURATION] = "configuration",
  [USB_IDX_INTERFACE] = "interface",
};

/* Messages used in printing transfer types. */
static const char *tfr_info[] = {
  [USB_TFR_CONTROL] = "control",
  [USB_TFR_ISOCHRONOUS] = "isochronous",
  [USB_TFR_BULK] = "bulk",
  [USB_TFR_INTERRUPT] = "interrupt",
};

/* Messages used in printing directions. */
static const char *dir_info[] = {
  [USB_DIR_INPUT] = "input",
  [USB_DIR_OUTPUT] = "output",
};

/* USB buffer used for USB transfers. */
struct usb_buf {
  ringbuf_t rb;            /* write source or read destination */
  condvar_t cv;            /* wait for the transfer to complete */
  spin_t lock;             /* buffer guard */
  usb_error_t error;       /* errors encountered during transfer */
  usb_transfer_t transfer; /* what kind of transfer is this? */
  usb_direction_t dir;     /* transfer direction */
  uint16_t transfer_size;  /* size of data to transfer in the data stage */
};

/*
 * USB buffer handling functions.
 */

void usb_buf_copyin(usb_buf_t *buf, void *src, size_t size) {
  ringbuf_t *rb = &buf->rb;
  assert(rb->size <= size);
  ringbuf_reset(rb);
  memcpy(rb->data, src, size);
}

void usb_buf_copyout(usb_buf_t *buf, void *dst, size_t size) {
  ringbuf_t *rb = &buf->rb;
  assert(rb->size >= size);
  memcpy(dst, rb->data, size);
}

usb_buf_t *usb_buf_alloc(size_t size) {
  usb_buf_t *buf = kmalloc(M_DEV, sizeof(usb_buf_t), M_ZERO);
  void *data = NULL;
  if (size)
    data = kmalloc(M_DEV, size, M_WAITOK);

  ringbuf_init(&buf->rb, data, size);
  cv_init(&buf->cv, "USB buffer ready");
  spin_init(&buf->lock, 0);

  return buf;
}

#define usb_buf_alloc_empty() usb_buf_alloc(0)

void usb_buf_free(usb_buf_t *buf) {
  ringbuf_t *rb = &buf->rb;
  if (rb->data)
    kfree(M_DEV, rb->data);
  kfree(M_DEV, buf);
}

/*
 * In case of an input transfer, data stage size is always non-zero, and we can
 * simply wait for `buf->transfer_size` bytes to appear in the waiting function.
 * However, an output transfer may have data stage size equal to zero.
 * In such cases we'll wait for `USB_BUF_MIN_WAIT_SIZE` to become the value of
 * ring buffer's count member. This value is non-zero, therefore we'll be
 * woken up.
 */
#define USB_BUF_MIN_WAIT_SIZE 1

/* Return the amount of bytes we'll wait to receive. */
static uint16_t usb_buf_wait_size(usb_buf_t *buf) {
  return max(buf->transfer_size, USB_BUF_MIN_WAIT_SIZE);
}

/* Wait until the transaction designated by `buf` is done,
 * or until an error occurs. */
static void usb_buf_wait(usb_buf_t *buf) {
  SCOPED_SPIN_LOCK(&buf->lock);

  ringbuf_t *rb = &buf->rb;
  uint16_t wait_size = usb_buf_wait_size(buf);
  while (!buf->error && !ringbuf_contains(rb, wait_size))
    cv_wait(&buf->cv, &buf->lock);
}

void usb_buf_process(usb_buf_t *buf, void *data, usb_error_t error) {
  SCOPED_SPIN_LOCK(&buf->lock);

  if (error) {
    buf->error |= error;
    goto end;
  }

  ringbuf_t *rb = &buf->rb;
  uint16_t wait_size = usb_buf_wait_size(buf);

  if (buf->dir == USB_DIR_INPUT) {
    /* Copy data to the buffer. Note, that if there isn't enough space
     * to accommodate the data, the data is lost. */
    ringbuf_putnb(rb, data, wait_size);
  } else {
    /* `count` reflects the amount of written data. */
    rb->count = wait_size;
  }

end:
  cv_signal(&buf->cv);
}

bool usb_buf_periodic(usb_buf_t *buf) {
  return buf->transfer == USB_TFR_INTERRUPT && buf->dir == USB_DIR_INPUT;
}

usb_error_t usb_buf_error(usb_buf_t *buf) {
  return buf->error;
}

usb_direction_t usb_buf_dir(usb_buf_t *buf) {
  return buf->dir;
}

uint16_t usb_buf_transfer_size(usb_buf_t *buf) {
  return buf->transfer_size;
}

int usb_buf_read(usb_buf_t *buf, void *dst, int flags) {
  assert(buf->dir == USB_DIR_INPUT);

  if (buf->error)
    return EIO;

  ringbuf_t *rb = &buf->rb;
  size_t size = buf->transfer_size;
  assert(size);

  /* Try to get the data. */
  if (ringbuf_getnb(rb, dst, size))
    return 0;

  /* If we can't wait, we won't. */
  if (flags & IO_NONBLOCK)
    return EAGAIN;

  /* Wait until the data is ready. */
  while (!buf->error && !ringbuf_getnb(rb, dst, size))
    usb_buf_wait(buf);

  /* If an error has occured, let's just return `EIO` since further information
   * is available in `buf->error`. */
  int error = 0;
  if (buf->error)
    error = EIO;

  return error;
}

int usb_buf_write(usb_buf_t *buf, int flags) {
  assert(buf->dir == USB_DIR_OUTPUT);

  if (buf->error)
    return EIO;

  ringbuf_t *rb = &buf->rb;
  size_t size = usb_buf_wait_size(buf);

  /* Data has been written iff `rb->count` is equal to `size`. */
  if (rb->count == size)
    return 0;

  /* If we can't wait, we won't. */
  if (flags & IO_NONBLOCK)
    return EAGAIN;

  /* Wait for the data to be written. */
  while (!buf->error && rb->count != size)
    usb_buf_wait(buf);

  /* If an error has occured, let's just return `EIO` since further information
   * is available in `buf->error`. */
  int error = 0;
  if (buf->error)
    error = EIO;

  return error;
}

/*
 * USB endpoint handling functions.
 */

/* Allocate an endpoint. */
static usb_endp_t *usb_endp_alloc(uint16_t maxpkt, uint8_t addr,
                                  usb_transfer_t transfer, usb_direction_t dir,
                                  uint8_t interval) {
  usb_endp_t *endp = kmalloc(M_DEV, sizeof(usb_endp_t), M_ZERO);

  endp->maxpkt = maxpkt;
  endp->addr = addr;
  endp->transfer = transfer;
  endp->dir = dir;
  endp->interval = interval;

  return endp;
}

/* Release an endpoint. */
static inline void usb_endp_free(usb_endp_t *endp) {
  kfree(M_DEV, endp);
}

/*
 * USB device handling functions.
 */

/* Allocate a new device attached to port `prot`. */
static usb_device_t *usb_dev_alloc(uint8_t port) {
  /* We'll need USB device and a corresponding index table. */
  size_t size = sizeof(usb_device_t) + USB_IDX_COUNT * sizeof(uint8_t);
  usb_device_t *udev = kmalloc(M_DEV, size, M_ZERO);
  TAILQ_INIT(&udev->endps);

  /* Each device supplies at least a control endpoint.
   * The control endpoint's max packet size is at least `USB_MAX_IPACKET`. */

  /* Input control endpoint. */
  usb_endp_t *endp =
    usb_endp_alloc(USB_MAX_IPACKET, 0, USB_TFR_CONTROL, USB_DIR_INPUT, 0);
  TAILQ_INSERT_TAIL(&udev->endps, endp, link);

  /* Outptu control endpoint. */
  endp = usb_endp_alloc(USB_MAX_IPACKET, 0, USB_TFR_CONTROL, USB_DIR_OUTPUT, 0);
  TAILQ_INSERT_TAIL(&udev->endps, endp, link);

  udev->port = port;
  return udev;
}

/* Release a device. */
static void usb_dev_free(usb_device_t *udev) {
  usb_endp_t *endp;
  TAILQ_FOREACH (endp, &udev->endps, link)
    usb_endp_free(endp);
  kfree(M_DEV, udev);
}

/* Return endpoint of device `dev` which implements transfer type `transfer`
 * with direction `dir`. */
static usb_endp_t *usb_dev_endp(usb_device_t *udev, usb_transfer_t transfer,
                                usb_direction_t dir) {
  /* XXX: we assume that only one enpoint matches
   * the pair (`transfer`, `dir`). */
  usb_endp_t *endp;
  TAILQ_FOREACH (endp, &udev->endps, link) {
    if (endp->transfer == transfer && endp->dir == dir)
      break;
  }
  return endp;
}

#define usb_dev_ctrl_endp(udev)                                                \
  usb_dev_endp((udev), USB_TFR_CONTROL, USB_DIR_INPUT | USB_DIR_OUTPUT)

/* Get string descriptor index at position `idx` in the device's index table. */
static inline uint8_t usb_dev_get_idx(usb_device_t *udev, usb_idx_t idx) {
  return ((uint8_t *)(udev + 1))[idx];
}

/* Set string descriptor index at position `idx` in the device's index table
 * to vale `str_idx`. */
static inline void usb_dev_set_idx(usb_device_t *udev, usb_idx_t idx,
                                   uint8_t str_idx) {
  ((uint8_t *)(udev + 1))[idx] = idx;
}

/*
 * USB transfer functions.
 */

static void _usb_control_transfer(device_t *dev, usb_buf_t *buf,
                                  usb_direction_t dir, usb_dev_req_t *req) {
  usb_device_t *udev = usb_device_of(dev);
  usb_endp_t *endp = usb_dev_ctrl_endp(udev);
  device_t *busdev = usb_bus_of(dev);

  /* Mark the buffer as being used for a transaction. */
  buf->error = 0; /* there is no error in the transaction yet */
  buf->dir = dir;
  buf->transfer = USB_TFR_CONTROL;
  buf->transfer_size = req->wLength;

  /* The corresponding host controller implements the actual transfer. */
  usbhc_control_transfer(busdev, endp->maxpkt, udev->port, udev->addr, buf,
                         req);
}

static void _usb_interrupt_transfer(device_t *dev, usb_buf_t *buf,
                                    usb_direction_t dir, uint16_t size) {
  usb_device_t *udev = usb_device_of(dev);
  usb_endp_t *endp = usb_dev_endp(udev, USB_TFR_INTERRUPT, dir);
  device_t *busdev = usb_bus_of(dev);

  /* Mark the buffer as being used for a transaction. */
  buf->error = 0; /* there is no error in the transaction yet */
  buf->dir = dir;
  buf->transfer = USB_TFR_INTERRUPT;
  buf->transfer_size = size;

  /* The corresponding host controller implements the actual transfer. */
  usbhc_interrupt_transfer(busdev, endp->maxpkt, udev->port, udev->addr,
                           endp->addr, endp->interval, buf);
}

static void _usb_bulk_transfer(device_t *dev, usb_buf_t *buf,
                               usb_direction_t dir, uint16_t size) {
  usb_device_t *udev = usb_device_of(dev);
  usb_endp_t *endp = usb_dev_endp(udev, USB_TFR_BULK, dir);
  device_t *busdev = usb_bus_of(dev);

  /* Mark the buffer as being used for a transaction. */
  buf->error = 0; /* there is no error in the transaction yet */
  buf->dir = dir;
  buf->transfer = USB_TFR_BULK;
  buf->transfer_size = size;

  /* The corresponding host controller implements the actual transfer. */
  usbhc_bulk_transfer(busdev, endp->maxpkt, udev->port, udev->addr, endp->addr,
                      buf);
}

/*
 * USB standard requests.
 */

/* Obtain device descriptor corresponding to device `dev`. */
static int usb_get_dev_dsc(device_t *dev, usb_dev_dsc_t *devdsc) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_DEVICE, 0),
    .wLength = sizeof(uint8_t),
  };
  usb_buf_t *buf = usb_buf_alloc(sizeof(usb_dev_dsc_t));

  /* The actual size of the descriptor is contained in the first byte,
   * hence we'll read it first. */
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  int error = usb_buf_read(buf, devdsc, 0);
  if (error)
    goto end;

  /* Get the whole descriptor. */
  req.wLength = devdsc->bLength;
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  error = usb_buf_read(buf, devdsc, 0);

end:
  usb_buf_free(buf);
  return error;
}

/* Assign the next available address in the USB bus to device `dev`. */
static int usb_set_addr(device_t *dev) {
  usb_state_t *usb = usb_bus_of(dev)->state;
  uint8_t addr = usb->next_addr++;
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_DEVICE,
    .bRequest = UR_SET_ADDRESS,
    .wValue = addr,
  };
  usb_buf_t *buf = usb_buf_alloc_empty();
  usb_control_transfer(dev, buf, USB_DIR_OUTPUT, &req);
  int error = usb_buf_write(buf, 0);
  if (!error)
    udev->addr = addr;

  usb_buf_free(buf);
  return error;
}

/*
 * Each device has one or more configuration descriptors.
 * A configuration descriptor consists of a header followed by all
 * the interface descriptors supplied by the device along with each endpoint
 * descriptor for each interface. Since most simple USB devices contain
 * only a single configuration with a single interface spaning a few endpoints,
 * we assume it to be the case.
 *
 * Conceptual drawing:
 *
 *   configuration start ----------------------
 *                       |   configuration    | Includes the total length
 *                       |       header       | of the configuration.
 *                       | (usb_config_dsc_t) |
 *                       ----------------------
 *                       |     interface      |
 *                       |     descriptor     | Includes number of endpoints.
 *                       |   (usb_if_dsc_t)   |
 *                       ----------------------
 *                      *|   HID descriptor   | HID devices only.
 *                      *|  (usb_hid_dsc_t)   |
 *                       ----------------------
 *                       |     endpoint 0     |
 *                       |    (usb_endp_t)    |
 *                       ----------------------
 *                       |                    |
 *                                ...
 *                       |                    |
 *                       ----------------------
 *                       |     endpoint n     |
 *                       |    (usb_endp_t)    |
 *    configuration end  ----------------------
 */

/* We assume that device's configuration is no larger
 * than `USB_MAX_CONFIG_SIZE`. */
#define USB_MAX_CONFIG_SIZE 0x30

/* Retreive device's configuration. */
static int usb_get_config(device_t *dev, usb_config_dsc_t *configdsc) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_CONFIG, 0 /* the first configuration */),
    .wLength = sizeof(uint32_t),
  };
  usb_buf_t *buf = usb_buf_alloc(USB_MAX_CONFIG_SIZE);

  /* First we'll read the total size of the configuration. */
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  int error = usb_buf_read(buf, configdsc, 0);
  if (error)
    goto end;

  /* Read the whole configuration. */
  req.wLength = configdsc->wTotalLength;
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  error = usb_buf_read(buf, configdsc, 0);

end:
  usb_buf_free(buf);
  return error;
}

/* Set device's configuration to the one identified
 * by configuration value `val.*/
static int usb_set_config(device_t *dev, uint8_t val) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE,
    .bRequest = UR_SET_CONFIG,
    .wValue = val,
  };
  usb_buf_t *buf = usb_buf_alloc_empty();
  usb_control_transfer(dev, buf, USB_DIR_OUTPUT, &req);
  int error = usb_buf_write(buf, 0);

  usb_buf_free(buf);
  return error;
}

static int _usb_unhalt_endp(device_t *dev, usb_transfer_t transfer,
                            usb_direction_t dir) {
  usb_device_t *udev = usb_device_of(dev);
  usb_endp_t *endp = usb_dev_endp(udev, transfer, dir);
  if (!endp)
    return EINVAL;

  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_ENDPOINT,
    .bRequest = UR_CLEAR_FEATURE,
    .wValue = UF_ENDPOINT_HALT,
    .wIndex = endp->addr,
  };
  usb_buf_t *buf = usb_buf_alloc_empty();
  usb_control_transfer(dev, buf, USB_DIR_OUTPUT, &req);
  int error = usb_buf_write(buf, 0);

  usb_buf_free(buf);
  return error;
}

/* Retreive deivice's string language descriptor. */
static int usb_get_str_lang_dsc(device_t *dev, usb_str_lang_t *langs) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_STRING, 0),
    .wIndex = USB_LANGUAGE_TABLE,
    .wLength = sizeof(uint8_t),
  };
  usb_buf_t *buf = usb_buf_alloc(sizeof(usb_str_lang_t));

  /* Size is contained in the first byte, so get it first. */
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  int error = usb_buf_read(buf, langs, 0);
  if (error)
    goto end;

  /* Read the whole language table. */
  req.wLength = langs->bLength;
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  error = usb_buf_read(buf, langs, 0);

end:
  usb_buf_free(buf);
  return error;
}

/* Fetch device's string descriptor designated identified by index `idx`. */
static int usb_get_str_dsc(device_t *dev, uint8_t idx, usb_str_dsc_t *strdsc) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_STRING, idx),
    .wIndex = US_ENG_LID,
    .wLength = sizeof(uint8_t),
  };
  usb_buf_t *buf = usb_buf_alloc(sizeof(usb_str_dsc_t));

  /* Obtain size of the descriptor. */
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  int error = usb_buf_read(buf, strdsc, 0);
  if (error)
    goto end;

  /* Read the whole descriptor. */
  req.wLength = strdsc->bLength;
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  error = usb_buf_read(buf, strdsc, 0);

end:
  usb_buf_free(buf);
  return error;
}

/*
 * USB HID standard requests.
 */

static int _usb_hid_set_idle(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_SET_IDLE,
    .wIndex = udev->ifnum,
  };
  usb_buf_t *buf = usb_buf_alloc_empty();
  usb_control_transfer(dev, buf, USB_DIR_OUTPUT, &req);
  int error = usb_buf_write(buf, 0);

  usb_buf_free(buf);
  return error;
}

static int _usb_hid_set_boot_protocol(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_SET_PROTOCOL,
    .wIndex = udev->ifnum,
  };
  usb_buf_t *buf = usb_buf_alloc_empty();
  usb_control_transfer(dev, buf, USB_DIR_OUTPUT, &req);
  int error = usb_buf_write(buf, 0);

  usb_buf_free(buf);
  return error;
}

/*
 * USB Bulk-Only standard requests.
 */

static int _usb_bbb_get_max_lun(device_t *dev, uint8_t *maxlun) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_CLASS_INTERFACE,
    .bRequest = UR_BBB_GET_MAX_LUN,
    .wIndex = udev->ifnum,
    .wLength = sizeof(uint8_t),
  };
  usb_buf_t *buf = usb_buf_alloc(sizeof(uint8_t));
  usb_control_transfer(dev, buf, USB_DIR_INPUT, &req);
  int error = usb_buf_read(buf, maxlun, 0);

  if (!error)
    goto end;

  /* A STALL means maxlun = 0. */
  if (buf->error == USB_ERR_STALLED) {
    *maxlun = 0;
    error = 0;
  }

end:
  usb_buf_free(buf);
  return error;
}

static int _usb_bbb_reset(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_BBB_RESET,
    .wIndex = udev->ifnum,
  };
  usb_buf_t *buf = usb_buf_alloc_empty();
  usb_control_transfer(dev, buf, USB_DIR_OUTPUT, &req);
  int error = usb_buf_write(buf, 0);

  usb_buf_free(buf);
  return error;
}

/*
 * Miscellaneous USB functions.
 */

usb_direction_t usb_status_dir(usb_direction_t dir, uint16_t transfer_size) {
  if (dir == USB_DIR_OUTPUT || !transfer_size)
    return USB_DIR_INPUT;
  return USB_DIR_OUTPUT;
}

/*
 * Printing related functions.
 */

/* Check whether language identified by `lid` is marked as supported
 * in string language descriptor `langs`. */
static bool usb_lang_supported(usb_str_lang_t *langs, uint16_t lid) {
  uint8_t nlangs = (langs->bLength - 2) / 2;

  for (uint8_t i = 0; i < nlangs; i++)
    if (langs->bData[i] == lid)
      return true;

  return false;
}

/* Check if device `dev` supports English. */
static int usb_english_support(device_t *dev, bool *supports) {
  usb_str_lang_t *langs = kmalloc(M_DEV, sizeof(usb_str_lang_t), M_ZERO);
  int error = usb_get_str_lang_dsc(dev, langs);
  if (error)
    goto end;

  *supports = usb_lang_supported(langs, US_ENG_LID);

  if (*supports)
    klog("device supports %s", US_ENG_STR);
  else
    klog("device doesn't support %s", US_ENG_STR);

end:
  kfree(M_DEV, langs);
  return error;
}

/* Since string descriptors use UTF-16 encoding, this function is used to
 * convert descriptor's data to a simple null terminated string. */
static char *usb_str_dsc2str(usb_str_dsc_t *strdsc) {
  uint8_t len = (strdsc->bLength - 2) / 2;
  char *buf = kmalloc(M_DEV, len + 1, M_WAITOK);

  uint8_t i = 0;
  for (; i < len; i++)
    buf[i] = strdsc->bString[i];
  buf[i] = 0;

  return buf;
}

/* Print string descriptor indicated by index `idx` preceded by message `msg`,
 * colon, and space. */
static int usb_print_single_str_dsc(device_t *dev, uint8_t idx,
                                    const char *msg) {
  assert(idx);

  usb_str_dsc_t *strdsc = kmalloc(M_DEV, sizeof(usb_str_dsc_t), M_ZERO);
  int error = usb_get_str_dsc(dev, idx, strdsc);
  if (error)
    goto end;

  char *str = usb_str_dsc2str(strdsc);
  klog("%s: %s", msg, str);
  kfree(M_DEV, str);

end:
  kfree(M_DEV, strdsc);
  return error;
}

/* Print string descriptors associated with device `dev`. */
static int usb_print_all_str_dsc(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);

  for (usb_idx_t idx = 0; idx < USB_IDX_COUNT; idx++) {
    uint8_t str_idx = usb_dev_get_idx(udev, idx);

    /* Valid string descriptors have index >= 1. */
    if (!str_idx)
      continue;

    int error = usb_print_single_str_dsc(dev, str_idx, idx_info[idx]);
    if (error)
      return error;
  }

  return 0;
}

/* Print endpoints supplied by device `dev`. */
static void usb_print_endps(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);

  usb_endp_t *endp;
  TAILQ_FOREACH (endp, &udev->endps, link) {
    klog("endpoint %hhd", endp->addr);
    klog("max packet size %hd", endp->maxpkt);
    klog("transfer type: %s", tfr_info[endp->transfer]);
    klog("direction: %s", dir_info[endp->dir]);
    if (endp->interval)
      klog("interval: %hhd", endp->interval);
    else
      klog("no polling required");
  }
}

/* Print information regarding device `dev`. */
static int usb_print_dev(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);

  klog("address: %02hhx", udev->addr);
  klog("device class code: %02hhx", udev->class_code);
  klog("device subclass code: %02hhx", udev->subclass_code);
  klog("device protocol code: %02hhx", udev->protocol_code);
  klog("vendor ID: %04hx", udev->vendor_id);
  klog("product ID: %04hx", udev->product_id);

  bool eng;
  int error = usb_english_support(dev, &eng);
  if (error || !eng)
    return error;

  if ((error = usb_print_all_str_dsc(dev)))
    return error;

  usb_print_endps(dev);

  return error;
}

/*
 * USB device enumeration and configuration functions.
 */

/* Get some basic device information and move it to the addressed stage
 * from where it can be further configured. */
static int usb_identify(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_dsc_t *devdsc = kmalloc(M_DEV, sizeof(usb_dev_dsc_t), M_ZERO);
  int error = usb_get_dev_dsc(dev, devdsc);
  if (error)
    goto end;

  /* If `bDeviceClass` field is 0, the class, subclass, and protocol codes
   * should be retreived form an interface descriptor. */
  if (devdsc->bDeviceClass) {
    udev->class_code = devdsc->bDeviceClass;
    udev->subclass_code = devdsc->bDeviceSubClass;
    udev->protocol_code = devdsc->bDeviceProtocol;
  }

  /* Update endpoint zero's max packet size. */
  usb_endp_t *endp = usb_dev_ctrl_endp(udev);
  endp->maxpkt = devdsc->bMaxPacketSize;

  udev->vendor_id = devdsc->idVendor;
  udev->product_id = devdsc->idProduct;

  /* Save string descriptors. */
  usb_dev_set_idx(udev, USB_IDX_MANUFACTURER, devdsc->iManufacturer);
  usb_dev_set_idx(udev, USB_IDX_PRODUCT, devdsc->iProduct);
  usb_dev_set_idx(udev, USB_IDX_SERIAL_NUMBER, devdsc->iSerialNumber);

  /* Assign a unique address to the device. */
  error = usb_set_addr(dev);

end:
  kfree(M_DEV, devdsc);
  return error;
}

/* Get the interface descriptor from a configuration descriptor. */
static inline usb_if_dsc_t *usb_config_if_dsc(usb_config_dsc_t *configdsc) {
  return (usb_if_dsc_t *)(configdsc + 1);
}

/* Return address of the first endpoint within interface `ifdsc`. */
static usb_endp_dsc_t *usb_if_endp_dsc(usb_if_dsc_t *ifdsc) {
  void *addr = ifdsc + 1;
  if (ifdsc->bInterfaceClass == UICLASS_HID) {
    usb_hid_dsc_t *hiddsc = addr;
    addr += hiddsc->bLength;
  }
  return (usb_endp_dsc_t *)addr;
}

/* Process each endpoint implemented by interface `ifdsc`
 * within device `udev`. */
static void usb_if_process_endps(usb_if_dsc_t *ifdsc, usb_device_t *udev) {
  usb_endp_dsc_t *endpdsc = usb_if_endp_dsc(ifdsc);

  for (uint8_t i = 0; i < ifdsc->bNumEndpoints; i++, endpdsc++) {
    /* Obtain endpoint's address. */
    uint8_t addr = UE_GET_ADDR(endpdsc->bEndpointAddress);

    /* Obtain endpoint's direction. */
    usb_direction_t dir = USB_DIR_OUTPUT;
    if (UE_GET_DIR(endpdsc->bEndpointAddress))
      dir = USB_DIR_INPUT;

    /* Obtain endpoint's transfer type. */
    uint8_t attr = endpdsc->bmAttributes;
    usb_transfer_t transfer = UE_TRANSFER_TYPE(attr) + 1;

    /* Add a new endpoint to the device. */
    usb_endp_t *endp = usb_endp_alloc(endpdsc->wMaxPacketSize, addr, transfer,
                                      dir, endpdsc->bInterval);
    TAILQ_INSERT_TAIL(&udev->endps, endp, link);
  }
}

/* Move device `dev` form addressed to configured state. Layout of device
 * descriptor is described beside `usb_get_config` definition. */
static int usb_configure(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_config_dsc_t *configdsc = kmalloc(M_DEV, USB_MAX_CONFIG_SIZE, M_ZERO);
  int error = usb_get_config(dev, configdsc);
  if (error)
    return error;

  /* Save configuration string descriptor index. */
  usb_dev_set_idx(udev, USB_IDX_CONFIGURATION, configdsc->iConfiguration);

  usb_if_dsc_t *ifdsc = usb_config_if_dsc(configdsc);

  /* Fill device codes if necessarry. */
  if (!udev->class_code) {
    udev->class_code = ifdsc->bInterfaceClass;
    udev->subclass_code = ifdsc->bInterfaceSubClass;
    udev->protocol_code = ifdsc->bInterfaceProtocol;
  }

  /* As we assume only a single interface, remember it's identifier. */
  udev->ifnum = ifdsc->bInterfaceNumber;

  /* Save interface string descriptor index. */
  usb_dev_set_idx(udev, USB_IDX_INTERFACE, ifdsc->iInterface);

  /* Process each supplied endpoint. */
  usb_if_process_endps(ifdsc, udev);

  /* Move the device to the configured state. */
  error = usb_set_config(dev, configdsc->bConfigurationValue);

  return error;
}

/* Create and add a new child device attached to port `port`
 * to USB bus device `busdev`. */
static device_t *usb_add_child(device_t *busdev, uint8_t port) {
  device_t *dev = device_add_child(busdev, port);
  dev->bus = DEV_BUS_USB;
  dev->instance = usb_dev_alloc(port);
  return dev;
}

/* Remove USB bus's device `dev`. */
static void usb_remove_child(device_t *busdev, device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  uint8_t port = udev->port;
  usb_dev_free(udev);
  device_remove_child(busdev, port);
}

int usb_enumerate(device_t *hcdev) {
  device_t *busdev = usb_bus_of(hcdev);
  uint8_t nports = usbhc_number_of_ports(hcdev);

  /* Identify and configure each device attached to the root hub. */
  for (uint8_t port = 0; port < nports; port++) {
    int error = 0;
    usbhc_reset_port(hcdev, port);

    /* If there is no device attached, step to the next port. */
    if (!usbhc_device_present(hcdev, port)) {
      klog("no device attached to port %hhu", port);
      continue;
    }
    klog("device attached to port %hhu", port);

    /* We'll perform some requests on device's behalf so lets create
     * its software representation. */
    device_t *dev = usb_add_child(busdev, port);

    if ((error = usb_identify(dev))) {
      klog("failed to identify the device at port %hhu", port);
      goto bad;
    }

    if ((error = usb_configure(dev))) {
      klog("failed to configure the device at port %hhu", port);
      goto bad;
    }

    if ((error = usb_print_dev(dev))) {
      klog("failed to (language) string descriptor");
      goto bad;
    }

    continue;

  bad:
    usb_remove_child(busdev, dev);
    return error;
  }

  /* Now, each valid attached device is configured and ready to perform
   * device specific requests. The next step is to match them with
   * corresponding device drivers. */
  return bus_generic_probe(busdev);
}

/*
 * USB bus initialization.
 */

DEVCLASS_CREATE(usb);

static driver_t usb_bus;

void usb_init(device_t *hcdev) {
  device_t *busdev = device_add_child(hcdev, 0);
  busdev->driver = &usb_bus;
  busdev->devclass = &DEVCLASS(usb);
  (void)device_probe(busdev);
  (void)device_attach(busdev);
}

static int usb_probe(device_t *busdev) {
  /* Since the calling scheme is special, just return best fit indicator. */
  return 1;
}

static int usb_attach(device_t *busdev) {
  usb_state_t *usb = busdev->state;
  /* Address 0 is special and reserved. */
  usb->next_addr = 1;
  return 0;
}

/* USB bus standard interface. */
static usb_methods_t usb_if = {
  .control_transfer = _usb_control_transfer,
  .interrupt_transfer = _usb_interrupt_transfer,
  .bulk_transfer = _usb_bulk_transfer,
};

/* USB standard requests interface. */
static usb_req_methods_t usb_req_if = {
  .unhalt_endp = _usb_unhalt_endp,
};

/* USB HID specific standard requests interface. */
static usb_hid_methods_t usb_hid_if = {
  .set_idle = _usb_hid_set_idle,
  .set_boot_protocol = _usb_hid_set_boot_protocol,
};

/* USB Bulk-Only specific standard requests interface. */
static usb_bbb_methods_t usb_bbb_if = {
  .get_max_lun = _usb_bbb_get_max_lun,
  .reset = _usb_bbb_reset,
};

static driver_t usb_bus = {
  .desc = "USB bus driver",
  .size = sizeof(usb_state_t),
  .probe = usb_probe,
  .attach = usb_attach,
  .interfaces =
    {
      [DIF_USB] = &usb_if,
      [DIF_USB_REQ] = &usb_req_if,
      [DIF_USB_HID] = &usb_hid_if,
      [DIF_USB_BBB] = &usb_bbb_if,
    },
};

/* TODO: remove this when merging any USB device driver. */
static driver_t usb_dev_driver_stub;
DEVCLASS_ENTRY(usb, usb_dev_driver_stub);
