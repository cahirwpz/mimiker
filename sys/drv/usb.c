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
#include <sys/bus.h>
#include <dev/usb.h>
#include <dev/usbhc.h>
#include <dev/usbhid.h>
#include <dev/umass.h>

/* USB bus software state. */
typedef struct usb_state {
  uint8_t next_addr; /* next device address to grant */
} usb_state_t;

/* Messages used in printing string descriptors. */
static const char *ustr_info[] = {
  [USB_STR_MANUFACTURER] = "manufacturer",
  [USB_STR_PRODUCT] = "product",
  [USB_STR_SERIAL_NUMBER] = "serial number",
  [USB_STR_CONFIGURATION] = "configuration",
  [USB_STR_INTERFACE] = "interface",
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

/* Messages used in printing device speeds. */
static const char *speed_info[] = {
  [USB_SPD_LOW] = "low",
  [USB_SPD_FULL] = "full",
};

/*
 * USB buffer handling functions.
 */

usb_buf_t *usb_buf_alloc(void) {
  usb_buf_t *buf = kmalloc(M_DEV, sizeof(usb_buf_t), M_ZERO);

  cv_init(&buf->cv, "USB buffer ready");
  spin_init(&buf->lock, 0);

  return buf;
}

bool usb_buf_periodic(usb_buf_t *buf) {
  usb_endpt_t *endpt = buf->endpt;
  return endpt->transfer == USB_TFR_INTERRUPT && endpt->dir == USB_DIR_INPUT;
}

void usb_buf_free(usb_buf_t *buf) {
  if (usb_buf_periodic(buf)) {
    assert(buf->priv);
    kfree(M_DEV, buf->priv);
  }
  kfree(M_DEV, buf);
}

/* Prepare buffer `buf` for a new transaction. */
static void usb_buf_prepare(usb_buf_t *buf, usb_endpt_t *endpt, void *data,
                            uint16_t transfer_size) {
  buf->endpt = endpt;
  buf->data = data;

  /* Set the `priv` member. */
  if (usb_buf_periodic(buf)) {
    if (!buf->priv) {
      buf->priv = kmalloc(M_DEV, transfer_size, M_WAITOK);
    } else if (buf->transfer_size != transfer_size) {
      kfree(M_DEV, buf->priv);
      buf->priv = kmalloc(M_DEV, transfer_size, M_WAITOK);
    }
    assert(buf->priv);
  } else if (buf->priv) {
    kfree(M_DEV, buf->priv);
    buf->priv = NULL;
  }

  buf->executed = 0;
  buf->transfer_size = transfer_size;
  buf->error = 0; /* there is no error in the transaction yet */
}

int usb_buf_wait(usb_buf_t *buf) {
  SCOPED_SPIN_LOCK(&buf->lock);

  while (!buf->error && !buf->executed)
    cv_wait(&buf->cv, &buf->lock);

  /* If an error has occured, let's just return `EIO`
   * since further information is available in `buf->error`. */
  if (buf->error)
    return EIO;

  /* In case of periodic transfers, hand data to the user. */
  if (usb_buf_periodic(buf)) {
    assert(buf->priv);
    memcpy(buf->data, buf->priv, buf->transfer_size);
  }

  buf->executed = 0;
  return 0;
}

void usb_buf_process(usb_buf_t *buf, void *data, usb_error_t error) {
  SCOPED_SPIN_LOCK(&buf->lock);

  if (error) {
    buf->error |= error;
    goto end;
  }

  void *dst = buf->data;
  usb_endpt_t *endpt = buf->endpt;
  if (endpt->dir == USB_DIR_INPUT) {
    /* In case of periodic transfers, copy the data
     * to the internal buffer first. */
    if (usb_buf_periodic(buf))
      dst = buf->priv;
    memcpy(dst, data, buf->transfer_size);
  }
  buf->executed = 1;

end:
  cv_signal(&buf->cv);
}

/*
 * USB endpoint handling functions.
 */

/* Allocate an endpoint. */
static usb_endpt_t *usb_endpt_alloc(uint16_t maxpkt, uint8_t addr,
                                    usb_transfer_t transfer,
                                    usb_direction_t dir, uint8_t interval) {
  usb_endpt_t *endpt = kmalloc(M_DEV, sizeof(usb_endpt_t), M_ZERO);

  endpt->maxpkt = maxpkt;
  endpt->addr = addr;
  endpt->transfer = transfer;
  endpt->dir = dir;
  endpt->interval = interval;

  return endpt;
}

/* Release an endpoint. */
static void usb_endpt_free(usb_endpt_t *endpt) {
  kfree(M_DEV, endpt);
}

/*
 * USB device handling functions.
 */

static usb_device_t *usb_dev_alloc(usb_speed_t speed) {
  usb_device_t *udev = kmalloc(M_DEV, sizeof(usb_device_t), M_ZERO);
  TAILQ_INIT(&udev->endpts);

  /* Each device supplies at least a control endpoint.
   * The control endpoint's max packet size is at least `USB_MAX_IPACKET`. */

  /* Input control endpoint. */
  usb_endpt_t *endpt =
    usb_endpt_alloc(USB_MAX_IPACKET, 0, USB_TFR_CONTROL, USB_DIR_INPUT, 0);
  TAILQ_INSERT_TAIL(&udev->endpts, endpt, link);

  /* Outptu control endpoint. */
  endpt =
    usb_endpt_alloc(USB_MAX_IPACKET, 0, USB_TFR_CONTROL, USB_DIR_OUTPUT, 0);
  TAILQ_INSERT_TAIL(&udev->endpts, endpt, link);

  udev->speed = speed;
  return udev;
}

/* Release a device. */
static void usb_dev_free(usb_device_t *udev) {
  usb_endpt_t *endpt;
  TAILQ_FOREACH (endpt, &udev->endpts, link)
    usb_endpt_free(endpt);
  kfree(M_DEV, udev);
}

/* Return endpoint of device `dev` which implements transfer type `transfer`
 * with direction `dir`. */
static usb_endpt_t *usb_dev_endpt(usb_device_t *udev, usb_transfer_t transfer,
                                  usb_direction_t dir) {
  /* XXX: we assume that only one enpoint matches
   * the pair (`transfer`, `dir`). */
  usb_endpt_t *endpt;
  TAILQ_FOREACH (endpt, &udev->endpts, link) {
    if (endpt->transfer == transfer && endpt->dir == dir)
      break;
  }
  return endpt;
}

#define usb_dev_ctrl_endpt(udev, dir)                                          \
  usb_dev_endpt((udev), USB_TFR_CONTROL, (dir))

/*
 * USB transfer functions.
 */

static void _usb_control_transfer(device_t *dev, usb_buf_t *buf, void *data,
                                  usb_direction_t dir, usb_dev_req_t *req) {
  usb_device_t *udev = usb_device_of(dev);
  usb_endpt_t *endpt = usb_dev_ctrl_endpt(udev, dir);
  assert(endpt);

  usb_buf_prepare(buf, endpt, data, req->wLength);

  /* The corresponding host controller implements the actual transfer. */
  usbhc_control_transfer(dev, buf, req);
}

static void _usb_data_transfer(device_t *dev, usb_buf_t *buf, void *data,
                               uint16_t size, usb_transfer_t transfer,
                               usb_direction_t dir) {
  usb_device_t *udev = usb_device_of(dev);
  usb_endpt_t *endpt = usb_dev_endpt(udev, transfer, dir);
  assert(endpt);

  usb_buf_prepare(buf, endpt, data, size);

  /* The corresponding host controller implements the actual transfer. */
  usbhc_data_transfer(dev, buf);
}

/*
 * USB standard requests.
 */

/* Perform a standard USB request. */
static int usb_send_req(device_t *dev, void *data, usb_direction_t dir,
                        usb_dev_req_t *req, usb_error_t *uerr_p) {
  usb_buf_t *buf = usb_buf_alloc();
  assert(buf);

  usb_control_transfer(dev, buf, data, dir, req);
  int error = usb_buf_wait(buf);
  if (uerr_p)
    *uerr_p = buf->error;

  usb_buf_free(buf);
  return error;
}

/* Obtain device descriptor corresponding to device `dev`. */
static int usb_get_dev_dsc(device_t *dev, usb_dev_dsc_t *devdsc) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_DEVICE, 0),
    .wLength = sizeof(uint8_t),
  };

  /* The actual size of the descriptor is contained in the first byte,
   * hence we'll read it first. */
  int error = usb_send_req(dev, devdsc, USB_DIR_INPUT, &req, NULL);
  if (error)
    return error;

  /* Get the whole descriptor. */
  req.wLength = devdsc->bLength;
  return usb_send_req(dev, devdsc, USB_DIR_INPUT, &req, NULL);
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

  int error = usb_send_req(dev, NULL, USB_DIR_OUTPUT, &req, NULL);
  if (error)
    return error;

  udev->addr = addr;

  return 0;
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
 *                       |  (usb_cfg_dsc_t)   |
 *                       ----------------------
 *                       |     interface      |
 *                       |     descriptor     | Includes number of endpoints.
 *                       |   (usb_if_dsc_t)   |
 *                       ----------------------
 *                      *|   HID descriptor   | HID devices only.
 *                      *|  (usb_hid_dsc_t)   |
 *                       ----------------------
 *                       |     endpoint 0     |
 *                       |   (usb_endpt_t)    |
 *                       ----------------------
 *                       |                    |
 *                                ...
 *                       |                    |
 *                       ----------------------
 *                       |     endpoint n     |
 *                       |   (usb_endpt_t)    |
 *    configuration end  ----------------------
 */

/* We assume that device's configuration is no larger
 * than `USB_MAX_CONFIG_SIZE`. */
#define USB_MAX_CONFIG_SIZE 0x30

/* Retreive device's configuration. */
static int usb_get_config(device_t *dev, usb_cfg_dsc_t *cfgdsc) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_CONFIG, 0 /* the first configuration */),
    .wLength = sizeof(uint32_t),
  };

  /* First we'll read the total size of the configuration. */
  int error = usb_send_req(dev, cfgdsc, USB_DIR_INPUT, &req, NULL);
  if (error)
    return error;

  /* Read the whole configuration. */
  req.wLength = cfgdsc->wTotalLength;
  return usb_send_req(dev, cfgdsc, USB_DIR_INPUT, &req, NULL);
}

/* Set device's configuration to the one identified
 * by configuration value `val.*/
static int usb_set_config(device_t *dev, uint8_t val) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE,
    .bRequest = UR_SET_CONFIG,
    .wValue = val,
  };
  return usb_send_req(dev, NULL, USB_DIR_OUTPUT, &req, NULL);
}

int usb_unhalt_endpt(device_t *dev, usb_transfer_t transfer,
                     usb_direction_t dir) {
  usb_device_t *udev = usb_device_of(dev);
  usb_endpt_t *endpt = usb_dev_endpt(udev, transfer, dir);
  if (!endpt)
    return EINVAL;

  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_ENDPOINT,
    .bRequest = UR_CLEAR_FEATURE,
    .wValue = UF_ENDPOINT_HALT,
    .wIndex = endpt->addr,
  };
  return usb_send_req(dev, NULL, USB_DIR_OUTPUT, &req, NULL);
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

  /* Size is contained in the first byte, so get it first. */
  int error = usb_send_req(dev, langs, USB_DIR_INPUT, &req, NULL);
  if (error)
    return error;

  /* Read the whole language table. */
  req.wLength = langs->bLength;
  return usb_send_req(dev, langs, USB_DIR_INPUT, &req, NULL);
}

/* Fetch device's string descriptor identified by index `idx`. */
static int usb_get_str_dsc(device_t *dev, uint8_t idx, usb_str_dsc_t *strdsc) {
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_STRING, idx),
    .wIndex = US_ENG_LID,
    .wLength = sizeof(uint8_t),
  };

  /* Obtain size of the descriptor. */
  int error = usb_send_req(dev, strdsc, USB_DIR_INPUT, &req, NULL);
  if (error)
    return error;

  /* Read the whole descriptor. */
  req.wLength = strdsc->bLength;
  return usb_send_req(dev, strdsc, USB_DIR_INPUT, &req, NULL);
}

/*
 * USB HID standard requests.
 */

int usb_hid_set_idle(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_SET_IDLE,
    .wIndex = udev->ifnum,
  };
  return usb_send_req(dev, NULL, USB_DIR_OUTPUT, &req, NULL);
}

int usb_hid_set_boot_protocol(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_SET_PROTOCOL,
    .wIndex = udev->ifnum,
  };
  return usb_send_req(dev, NULL, USB_DIR_OUTPUT, &req, NULL);
}

/*
 * USB Bulk-Only standard requests.
 */

int usb_bbb_get_max_lun(device_t *dev, uint8_t *maxlun) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_READ_CLASS_INTERFACE,
    .bRequest = UR_BBB_GET_MAX_LUN,
    .wIndex = udev->ifnum,
    .wLength = sizeof(uint8_t),
  };

  usb_error_t uerr;
  int error = usb_send_req(dev, maxlun, USB_DIR_INPUT, &req, &uerr);
  if (!error)
    return 0;

  /* A STALL means maxlun = 0. */
  if (uerr == USB_ERR_STALLED) {
    *maxlun = 0;
    error = 0;
  }

  return error;
}

int usb_bbb_reset(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_dev_req_t req = (usb_dev_req_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_BBB_RESET,
    .wIndex = udev->ifnum,
  };
  return usb_send_req(dev, NULL, USB_DIR_OUTPUT, &req, NULL);
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
static int usb_english_support(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_str_lang_t *langs = kmalloc(M_DEV, sizeof(usb_str_lang_t), M_ZERO);
  int error = usb_get_str_lang_dsc(dev, langs);
  if (error)
    goto end;

  if (usb_lang_supported(langs, US_ENG_LID)) {
    udev->eng_support = 1;
    klog("device supports %s", US_ENG_STR);
  } else {
    udev->eng_support = 0;
    klog("device doesn't support %s", US_ENG_STR);
  }

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

/* Obtain and save a string containing the contents of the string descriptor
 * identified by index `idx`. */
static int usb_get_str(device_t *dev, usb_str_t ustr, uint8_t idx) {
  usb_device_t *udev = usb_device_of(dev);
  assert(ustr < USB_STR_COUNT);

  /* If device doesn't support the English language, then we don't
   * care about string descriptors. */
  if (!udev->eng_support)
    return 0;

  /* Valid string descriptors have index >= 1. */
  if (!idx)
    return 0;

  usb_str_dsc_t *strdsc = kmalloc(M_DEV, sizeof(usb_str_dsc_t), M_ZERO);
  int error = usb_get_str_dsc(dev, idx, strdsc);
  if (error)
    goto end;

  udev->strs[ustr] = usb_str_dsc2str(strdsc);

end:
  kfree(M_DEV, strdsc);
  return error;
}

/* Print information regarding device `dev`.
 * TODO: this function should be replaced with a GDB python script. */
static void usb_print_dev(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);

  klog("device address: %u", udev->addr);
  klog("device class code: %#x", udev->class_code);
  klog("device subclass code: %#x", udev->subclass_code);
  klog("device protocol code: %#x", udev->protocol_code);
  klog("vendor ID: %#x", udev->vendor_id);
  klog("product ID: %#x", udev->product_id);
  klog("speed: %s", speed_info[udev->speed]);

  /* Print the provided string descriptors. */
  for (usb_str_t ustr = 0; ustr < USB_STR_COUNT; ustr++) {
    char *str = udev->strs[ustr];
    if (str)
      klog("%s: %s", str, ustr_info[ustr]);
  }

  /* Print the device's endpoints. */
  usb_endpt_t *endpt;
  TAILQ_FOREACH (endpt, &udev->endpts, link) {
    klog("endpoint address: %u", endpt->addr);
    klog("max packet size: %#x", endpt->maxpkt);
    klog("transfer type: %s", tfr_info[endpt->transfer]);
    klog("direction: %s", dir_info[endpt->dir]);
    if (endpt->interval)
      klog("interval: %u", endpt->interval);
    else
      klog("no polling required");
  }
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
  usb_endpt_t *endpt = usb_dev_ctrl_endpt(udev, USB_DIR_INPUT);
  endpt->maxpkt = devdsc->bMaxPacketSize;
  endpt = usb_dev_ctrl_endpt(udev, USB_DIR_OUTPUT);
  endpt->maxpkt = devdsc->bMaxPacketSize;

  udev->vendor_id = devdsc->idVendor;
  udev->product_id = devdsc->idProduct;

  /* Assign a unique address to the device. */
  if ((error = usb_set_addr(dev)))
    goto end;

  /* Check whether the device supports English before reading any
   * string descriptors. */
  if ((error = usb_english_support(dev)))
    goto end;

  /* Get the string descriptors. */
  if ((error = usb_get_str(dev, USB_STR_MANUFACTURER, devdsc->iManufacturer)))
    goto end;
  if ((error = usb_get_str(dev, USB_STR_PRODUCT, devdsc->iProduct)))
    goto end;
  if ((error = usb_get_str(dev, USB_STR_SERIAL_NUMBER, devdsc->iSerialNumber)))
    goto end;

end:
  kfree(M_DEV, devdsc);
  return error;
}

/* Return address of the first endpoint within interface `ifdsc`. */
static usb_endpt_dsc_t *usb_if_endpt_dsc(usb_if_dsc_t *ifdsc) {
  void *addr = ifdsc + 1;
  if (ifdsc->bInterfaceClass == UICLASS_HID) {
    usb_hid_dsc_t *hiddsc = addr;
    addr += hiddsc->bLength;
  }
  return (usb_endpt_dsc_t *)addr;
}

/* Process each endpoint implemented by interface `ifdsc`
 * within device `udev`. */
static void usb_if_process_endpts(usb_if_dsc_t *ifdsc, usb_device_t *udev) {
  usb_endpt_dsc_t *endptdsc = usb_if_endpt_dsc(ifdsc);

  for (uint8_t i = 0; i < ifdsc->bNumEndpoints; i++, endptdsc++) {
    /* Obtain endpoint's address. */
    uint8_t addr = UE_GET_ADDR(endptdsc->bEndpointAddress);

    /* Obtain endpoint's direction. */
    usb_direction_t dir = USB_DIR_OUTPUT;
    if (UE_GET_DIR(endptdsc->bEndpointAddress))
      dir = USB_DIR_INPUT;

    /* Obtain endpoint's transfer type. */
    uint8_t attr = endptdsc->bmAttributes;
    usb_transfer_t transfer = UE_TRANSFER_TYPE(attr) + 1;

    /* Add a new endpoint to the device. */
    usb_endpt_t *endpt = usb_endpt_alloc(endptdsc->wMaxPacketSize, addr,
                                         transfer, dir, endptdsc->bInterval);
    TAILQ_INSERT_TAIL(&udev->endpts, endpt, link);
  }
}

/* Move device `dev` form addressed to configured state. Layout of device
 * descriptor is described beside `usb_get_config` definition. */
static int usb_configure(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  usb_cfg_dsc_t *cfgdsc = kmalloc(M_DEV, USB_MAX_CONFIG_SIZE, M_ZERO);
  int error = usb_get_config(dev, cfgdsc);
  if (error)
    return error;

  /* Save configuration string descriptor. */
  if ((error = usb_get_str(dev, USB_STR_CONFIGURATION, cfgdsc->iConfiguration)))
    return error;

  usb_if_dsc_t *ifdsc = (usb_if_dsc_t *)(cfgdsc + 1);

  /* Fill device codes if necessarry. */
  if (!udev->class_code) {
    udev->class_code = ifdsc->bInterfaceClass;
    udev->subclass_code = ifdsc->bInterfaceSubClass;
    udev->protocol_code = ifdsc->bInterfaceProtocol;
  }

  /* As we assume only a single interface, remember it's identifier. */
  udev->ifnum = ifdsc->bInterfaceNumber;

  /* Save interface string descriptor. */
  if ((error = usb_get_str(dev, USB_STR_INTERFACE, ifdsc->iInterface)))
    return error;

  /* Process each supplied endpoint. */
  usb_if_process_endpts(ifdsc, udev);

  /* Move the device to the configured state. */
  error = usb_set_config(dev, cfgdsc->bConfigurationValue);

  return error;
}

/* Create and add a new child device attached to port `port`
 * to USB bus device `busdev`. */
static device_t *usb_add_child(device_t *busdev, uint8_t port,
                               usb_speed_t speed) {
  device_t *dev = device_add_child(busdev, port);
  dev->bus = DEV_BUS_USB;
  dev->instance = usb_dev_alloc(speed);
  return dev;
}

/* Remove USB bus's device `dev`. */
static void usb_remove_child(device_t *busdev, device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);

  for (usb_str_t ustr = 0; ustr < USB_STR_COUNT; ustr++) {
    char *str = udev->strs[ustr];
    if (str)
      kfree(M_DEV, str);
  }

  usb_dev_free(udev);
  device_remove_child(busdev, dev);
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
      klog("no device attached to port %u", port);
      continue;
    }
    klog("device attached to port %u", port);

    /* We'll perform some requests on device's behalf so lets create
     * its software representation. */
    usb_speed_t speed = usbhc_device_speed(hcdev, port);
    device_t *dev = usb_add_child(busdev, port, speed);

    if ((error = usb_identify(dev))) {
      klog("failed to identify the device at port %u", port);
      goto bad;
    }

    if ((error = usb_configure(dev))) {
      klog("failed to configure the device at port %u", port);
      goto bad;
    }

    usb_print_dev(dev);
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
  .data_transfer = _usb_data_transfer,
};

static driver_t usb_bus = {
  .desc = "USB bus driver",
  .size = sizeof(usb_state_t),
  .probe = usb_probe,
  .attach = usb_attach,
  .interfaces =
    {
      [DIF_USB] = &usb_if,
    },
};

/* TODO: remove this when merging any USB device driver. */
static driver_t usb_dev_driver_stub;
DEVCLASS_ENTRY(usb, usb_dev_driver_stub);
