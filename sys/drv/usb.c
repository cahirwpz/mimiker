#define KL_LOG KL_USB
#include <sys/devclass.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/klog.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <dev/usbhid.h>
#include <dev/usb.h>
#include <dev/usbhc.h>
#include <dev/umass.h>

/* TODO(MichalBlk): in case of multiple host controllers, this needs
 * protection. */
static uint8_t usb_max_addr = 1;

static usb_endpoint_descriptor_t *usb_endp(usb_device_t *usbd,
                                           usb_buf_t *usbb) {
  transfer_flags_t tf = usbb->flags;
  usb_endpoint_descriptor_t *ed = NULL;

  if (tf & TF_CONTROL)
    return NULL;

  if (tf & TF_BULK)
    ed = usbd->endps + (tf & TF_INPUT ? 0 : 1);
  else if (tf & TF_INTERRUPT)
    ed = usbd->endps;

  assert(ed);
  return ed;
}

uint16_t usb_max_pkt_size(usb_device_t *usbd, usb_buf_t *usbb) {
  usb_endpoint_descriptor_t *ep = usb_endp(usbd, usbb);
  if (!ep)
    return usbd->dd.bMaxPacketSize;
  return ep->wMaxPacketSize;
}

uint8_t usb_endp_addr(usb_device_t *usbd, usb_buf_t *usbb) {
  usb_endpoint_descriptor_t *ep = usb_endp(usbd, usbb);
  if (!ep)
    return 0;
  return UE_GET_ADDR(ep->bEndpointAddress);
}

uint8_t usb_status_type(usb_buf_t *usbb) {
  assert(usbb->flags & TF_CONTROL);
  return (!(usbb->flags & TF_INPUT) || !usbb->transfer_size ? TF_INPUT : 0);
}

uint8_t usb_interval(usb_device_t *usbd, usb_buf_t *usbb) {
  usb_endpoint_descriptor_t *ep = usb_endp(usbd, usbb);
  if (!ep)
    return 0;
  return ep->bInterval;
}

void usb_process(usb_buf_t *usbb, void *data, transfer_flags_t err_flags) {
  WITH_SPIN_LOCK (&usbb->lock) {
    if (!err_flags) {
      if (usbb->flags & TF_INPUT)
        ringbuf_putnb(&usbb->buf, data, usbb->transfer_size);
      else
        usbb->buf.count = max(usbb->transfer_size, 1);
    } else {
      usbb->flags |= err_flags;
    }
    cv_signal(&usbb->cv);
  }
}

/* Alloc a device attached to port `prot`. */
static usb_device_t *usb_alloc_dev(uint8_t port) {
  usb_device_t *usbd = kmalloc(M_DEV, sizeof(usb_device_t), M_ZERO);
  usbd->dd.bMaxPacketSize = USB_MAX_IPACKET;
  usbd->port = port;
  return usbd;
}

static void usb_free_dev(usb_device_t *usbd) {
  if (usbd->endps)
    kfree(M_DEV, usbd->endps);
  kfree(M_DEV, usbd);
}

usb_buf_t *usb_alloc_buf(void *data, size_t size, transfer_flags_t flags,
                         uint16_t transfer_size) {
  usb_buf_t *usbb = kmalloc(M_DEV, sizeof(usb_buf_t), M_WAITOK);

  ringbuf_init(&usbb->buf, data, size);
  cv_init(&usbb->cv, "USB buffer ready");
  spin_init(&usbb->lock, 0);
  usbb->flags = flags;
  usbb->transfer_size = transfer_size;

  return usbb;
}

void usb_free_buf(usb_buf_t *usbb) {
  kfree(M_DEV, usbb);
}

void usb_reuse_buf(usb_buf_t *usbb, void *data, size_t size,
                   transfer_flags_t flags, uint16_t transfer_size) {
  ringbuf_init(&usbb->buf, data, size);
  usbb->flags = flags;
  usbb->transfer_size = transfer_size;
}

void usb_clean_error(usb_buf_t *usbb) {
  usbb->flags &= ~(TF_STALLED | TF_ERROR);
}

void usb_reset_buf(usb_buf_t *usbb, uint16_t transfer_size) {
  ringbuf_reset(&usbb->buf);
  usb_clean_error(usbb);
  usbb->transfer_size = transfer_size;
}

bool usb_transfer_error(usb_buf_t *usbb) {
  return usbb->flags & (TF_STALLED | TF_ERROR);
}

void usb_wait(usb_buf_t *usbb) {
  assert(spin_owned(&usbb->lock));

  ringbuf_t *rb = &usbb->buf;
  uint16_t wait_size = max(usbb->transfer_size, 1);
  while (!usb_transfer_error(usbb) && rb->count < wait_size)
    cv_wait(&usbb->cv, &usbb->lock);
}

static int usb_control_transfer(device_t *dev, usb_buf_t *usbb,
                                usb_device_request_t *req) {
  assert(usbb->flags & TF_CONTROL);

  usbhc_transfer(dev, usbb, req);

  WITH_SPIN_LOCK (&usbb->lock) { usb_wait(usbb); }
  /* In case of control transfers, we consider a STALL condition
   * to be an error, thereby we assume that each standard request
   * performed is suported by device. */
  if (usb_transfer_error(usbb))
    return 1;
  return 0;
}

int usb_set_idle(device_t *dev) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_SET_IDLE,
    .wIndex = usbd->inum,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(dev, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

int usb_set_boot_protocol(device_t *dev) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_SET_PROTOCOL,
    .wIndex = usbd->inum,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(dev, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

int usb_bbb_get_max_lun(device_t *dev, uint8_t *maxlun) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_READ_CLASS_INTERFACE,
    .bRequest = UR_BBB_GET_MAX_LUN,
    .wIndex = usbd->inum,
    .wLength = 1,
  };
  usb_buf_t *usbb = usb_alloc_buf_from_struct(maxlun, TF_INPUT | TF_CONTROL, 1);
  int error = usb_control_transfer(dev, usbb, &req);

  if (!error)
    goto bad;

  if (usbb->flags & TF_STALLED && !(usbb->flags & TF_ERROR)) {
    /* A STALL means maxlun = 0. */
    *maxlun = 0;
    error = 0;
  }

bad:
  usb_free_buf(usbb);
  return error;
}

int usb_bbb_reset(device_t *dev) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_WRITE_CLASS_INTERFACE,
    .bRequest = UR_BBB_RESET,
    .wIndex = usbd->inum,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(dev, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

int usb_unhalt_endp(device_t *dev, uint8_t idx) {
  usb_device_t *usbd = usb_device_of(dev);
  assert(idx < usbd->nendps);

  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_WRITE_ENDPOINT,
    .bRequest = UR_CLEAR_FEATURE,
    .wValue = UF_ENDPOINT_HALT,
    .wIndex = UE_GET_ADDR(usbd->endps[idx].bEndpointAddress),
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(dev, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

void usb_interrupt_transfer(device_t *dev, usb_buf_t *usbb) {
  assert(usbb->flags & TF_INTERRUPT);
  usbhc_transfer(dev, usbb, NULL);
}

int usb_poll(device_t *dev, usb_buf_t *usbb, uint8_t idx, void *buf,
             size_t size) {
  WITH_SPIN_LOCK (&usbb->lock) {
    for (;;) {
      usb_wait(usbb);

      if (!usb_transfer_error(usbb)) {
        ringbuf_getnb(&usbb->buf, buf, size);
        break;
      }

      if (usbb->flags & TF_STALLED && !(usbb->flags & TF_ERROR)) {
        usb_unhalt_endp(dev, idx);
        usb_clean_error(usbb);
        usb_interrupt_transfer(dev, usbb);
      } else {
        return 1;
      }
    }
  }
  return 0;
}

void usb_bulk_transfer(device_t *dev, usb_buf_t *usbb) {
  assert(usbb->flags & TF_BULK);
  usbhc_transfer(dev, usbb, NULL);
}

static int usb_set_addr(device_t *dev) {
  uint8_t addr = usb_max_addr++;
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_WRITE_DEVICE,
    .bRequest = UR_SET_ADDRESS,
    .wValue = addr,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = 0;

  if (!(error = usb_control_transfer(dev, usbb, &req))) {
    usbd->addr = addr;
    mdelay(2);
  }

  usb_free_buf(usbb);
  return error;
}

static int usb_get_dev_dsc(device_t *dev, usb_device_descriptor_t *dd) {
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_DEVICE, 0),
    .wLength = USB_MAX_IPACKET,
  };
  usb_buf_t *usbb =
    usb_alloc_buf_from_struct(dd, TF_INPUT | TF_CONTROL, USB_MAX_IPACKET);
  int error = 0;

  /* Read the first 8 bytes. */
  if ((error = usb_control_transfer(dev, usbb, &req)))
    goto end;

  /* Get the whole descriptor. */
  req.wLength = dd->bLength;
  usb_reset_buf(usbb, dd->bLength);
  error = usb_control_transfer(dev, usbb, &req);

end:
  usb_free_buf(usbb);
  return error;
}

static int usb_get_str_lang_dsc(device_t *dev, usb_string_lang_t *langs) {
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_STRING, 0),
    .wIndex = USB_LANGUAGE_TABLE,
    .wLength = 2,
  };
  usb_buf_t *usbb = usb_alloc_buf_from_struct(langs, TF_INPUT | TF_CONTROL, 2);
  int error = 0;

  /* Obtain size of the lang table. */
  if ((error = usb_control_transfer(dev, usbb, &req)))
    goto end;

  /* Read the whole lang table. */
  req.wLength = langs->bLength;
  usb_reset_buf(usbb, req.wLength);
  error = usb_control_transfer(dev, usbb, &req);

end:
  usb_free_buf(usbb);
  return error;
}

static int usb_find_lang(usb_string_lang_t *langs, uint16_t lid) {
  uint8_t nlangs = (langs->bLength - 2) / 2;

  for (uint8_t i = 0; i < nlangs; i++)
    if (langs->bData[i] == lid)
      return 0;

  return 1;
}

static int usb_english_support(device_t *dev) {
  usb_string_lang_t *langs =
    kmalloc(M_DEV, sizeof(usb_string_lang_t), M_WAITOK);
  int error = 0;

  if (!(error = usb_get_str_lang_dsc(dev, langs)))
    error = usb_find_lang(langs, US_ENG_LID);

  if (!error)
    klog("device supports %s language", US_ENG_STR);
  else
    klog("device doesn't support %s language", US_ENG_STR);

  kfree(M_DEV, langs);
  return error;
}

static int usb_get_str_dsc(device_t *dev, uint8_t idx,
                           usb_string_descriptor_t *sd) {
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_STRING, idx),
    .wIndex = US_ENG_LID,
    .wLength = 2,
  };
  usb_buf_t *usbb = usb_alloc_buf_from_struct(sd, TF_INPUT | TF_CONTROL, 2);
  int error = 0;

  /* Obtain the size of the descriptor. */
  if ((error = usb_control_transfer(dev, usbb, &req)))
    goto end;

  /* Read the whole descriptor. */
  req.wLength = sd->bLength;
  usb_reset_buf(usbb, req.wLength);
  error = usb_control_transfer(dev, usbb, &req);

end:
  usb_free_buf(usbb);
  return error;
}

/* String descriptors use the UTF-16 encoding. */
static void usb_print_str_dsc(usb_string_descriptor_t *sd, const char *msg) {
  char *buf = kmalloc(M_DEV, US_DATASIZE + 1, M_WAITOK);
  uint8_t len = (sd->bLength - 2) / 2;
  uint8_t i = 0;

  for (; i < len; i++)
    buf[i] = sd->bString[i];
  buf[i] = 0;

  klog("%s: %s", msg, buf);
}

static void usb_handle_str_dsc(device_t *dev, uint8_t idx, const char *msg) {
  if (!idx)
    return;

  usb_string_descriptor_t *sd =
    kmalloc(M_DEV, sizeof(usb_string_descriptor_t), M_WAITOK);
  if (!usb_get_str_dsc(dev, idx, sd))
    usb_print_str_dsc(sd, msg);

  kfree(M_DEV, sd);
}

#define CONFIG_SIZE 0x30

static int usb_get_config_dsc(device_t *dev, uint8_t num,
                              usb_config_descriptor_t *cd) {
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_READ_DEVICE,
    .bRequest = UR_GET_DESCRIPTOR,
    .wValue = UV_MAKE(UDESC_CONFIG, num),
    .wLength = 4,
  };
  usb_buf_t *usbb = usb_alloc_buf(cd, CONFIG_SIZE, TF_INPUT | TF_CONTROL, 4);
  int error = 0;

  /* Obtain the total size of the configuration. */
  if ((error = usb_control_transfer(dev, usbb, &req)))
    goto end;

  /* Read the whole configuration. */
  req.wLength = cd->wTotalLength;
  usb_reset_buf(usbb, req.wLength);
  error = usb_control_transfer(dev, usbb, &req);

end:
  usb_free_buf(usbb);
  return error;
}

static int usb_set_configuration(device_t *dev, uint8_t val) {
  usb_device_request_t req = (usb_device_request_t){
    .bmRequestType = UT_WRITE,
    .bRequest = UR_SET_CONFIG,
    .wValue = val,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(dev, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

static void usb_print_transfer_type(usb_endpoint_descriptor_t *ed) {
  uint8_t tt = UE_TRANSFER_TYPE(ed->bmAttributes);
  const char *str = NULL;

  if (tt == UE_CONTROL) {
    str = "Control";
  } else if (tt == UE_ISOCHRONOUS) {
    str = "Isochronous";
  } else if (tt == UE_BULK) {
    str = "Bulk";
  } else if (tt == UE_INTERRUPT) {
    str = "Interrupt";
  } else {
    str = "Unknown";
  }

  klog("transfer type: %s", str);
}

static void usb_print_dev(device_t *dev, usb_config_descriptor_t *cd,
                          usb_interface_descriptor_t *id) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_descriptor_t *dd = &usbd->dd;

  klog("USB release: %04hx", dd->bcdUSB);
  klog("device class: %02hhx", dd->bDeviceClass);
  klog("device subclass: %02hhx", dd->bDeviceSubClass);
  klog("device protocol: %02hhx", dd->bDeviceProtocol);
  klog("max config packet size: %02hhx", dd->bMaxPacketSize);
  klog("vendor ID: %04hx", dd->idVendor);
  klog("product ID: %04hx", dd->idProduct);
  klog("device release: %04hx", dd->bcdDevice);
  klog("number of configutarions: %02hhx", dd->bNumConfigurations);

  bool eng = !usb_english_support(dev);

  if (eng) {
    usb_handle_str_dsc(dev, dd->iManufacturer, "manufacturer");
    usb_handle_str_dsc(dev, dd->iProduct, "product");
    usb_handle_str_dsc(dev, dd->iSerialNumber, "serial number");
  }

  if (eng)
    usb_handle_str_dsc(dev, cd->iConfiguration, "configuration");
  klog("number of interfaces: %hhu", cd->bNumInterface);
  klog("maximum power consumption: %hhu mA", cd->bMaxPower * 2);

  if (eng)
    usb_handle_str_dsc(dev, id->iInterface, "interface");
  klog("number of endpoints: %hhu", id->bNumEndpoints);

  for (int i = 0; i < usbd->nendps; i++) {
    usb_endpoint_descriptor_t *ed = &usbd->endps[i];
    klog("endpoint %d:", i);
    usb_print_transfer_type(ed);
    klog("address: %hhu", UE_GET_ADDR(ed->bEndpointAddress));
    if (UE_GET_DIR(ed->bEndpointAddress))
      klog("direction: input");
    else
      klog("direction: output");
    klog("max packet size: %hu", ed->wMaxPacketSize);
    if (ed->bInterval)
      klog("interval: %hhu", ed->bInterval);
    else
      klog("no polling required");
  }
}

static usb_endpoint_descriptor_t *
usb_endp_dsc_addr(usb_interface_descriptor_t *id) {
  if (id->bInterfaceClass == UICLASS_HID) {
    usb_hid_descriptor_t *hd = (usb_hid_descriptor_t *)(id + 1);
    return (void *)hd + hd->bLength;
  }
  return (usb_endpoint_descriptor_t *)(id + 1);
}

static int usb_configure(device_t *dev) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_config_descriptor_t *cd = kmalloc(M_DEV, CONFIG_SIZE, M_WAITOK);
  int error = 0;

  if ((error = usb_get_config_dsc(dev, 0, cd)))
    goto end;

  usb_interface_descriptor_t *id = (usb_interface_descriptor_t *)(cd + 1);

  if (!(usbd->dd.bDeviceClass)) {
    usbd->dd.bDeviceClass = id->bInterfaceClass;
    usbd->dd.bDeviceSubClass = id->bInterfaceSubClass;
    usbd->dd.bDeviceProtocol = id->bInterfaceProtocol;
  }
  usbd->inum = id->bInterfaceNumber;

  usbd->nendps = id->bNumEndpoints;
  usbd->endps =
    kmalloc(M_DEV, sizeof(usb_endpoint_descriptor_t) * usbd->nendps, M_WAITOK);

  usb_endpoint_descriptor_t *ed = usb_endp_dsc_addr(id);
  for (int i = 0; i < usbd->nendps; i++)
    memcpy(&usbd->endps[i], &ed[i], sizeof(usb_endpoint_descriptor_t));

  if ((error = usb_set_configuration(dev, cd->bConfigurationValue)))
    goto end;

  usb_print_dev(dev, cd, id);

end:
  kfree(M_DEV, cd);
  return error;
}

static int usb_identify(device_t *dev) {
  usb_device_t *usbd = usb_device_of(dev);

  if (usb_get_dev_dsc(dev, &usbd->dd))
    return 1;

  usbhc_reset_port(dev->parent, usbd->port);

  if (usb_set_addr(dev))
    return 1;

  return 0;
}

void usb_enumerate(device_t *dev) {
  uint8_t nports = usbhc_number_of_ports(dev);

  for (uint8_t pn = 0; pn < nports; pn++) {
    usbhc_reset_port(dev, pn);

    if (!usbhc_device_present(dev, pn)) {
      klog("no device attached to port %hhu", pn);
      continue;
    }
    klog("device attached to port %hhu", pn);

    device_t *udev = device_add_child(dev, pn);
    udev->instance = usb_alloc_dev(pn);

    if (usb_identify(udev)) {
      klog("failed to identify the device at port %hhu", pn);
      goto bad;
    }

    if (usb_configure(udev)) {
      klog("failed to configure the device at port %hhu", pn);
      goto bad;
    }

    continue;

  bad:
    usb_free_dev(udev->instance);
    device_remove_child(dev, pn);
  }
}
