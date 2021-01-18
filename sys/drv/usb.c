#define KL_LOG KL_USB
#include <sys/devclass.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/klog.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <dev/hid.h>
#include <dev/usb.h>
#include <dev/usbhc.h>
#include <dev/umass.h>

USBHC_SPACE_DEFINE;

static uint8_t usb_max_addr = 1;

static usb_endp_dsc_t *usb_endp(usb_device_t *usbd, usb_buf_t *usbb) {
  transfer_flags_t tf = usbb->flags;
  usb_endp_dsc_t *ed = NULL;

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
  usb_endp_dsc_t *ep = usb_endp(usbd, usbb);
  if (!ep)
    return usbd->dd.max_pkt_size;
  return ep->max_pkt_size;
}

uint8_t usb_endp_addr(usb_device_t *usbd, usb_buf_t *usbb) {
  usb_endp_dsc_t *ep = usb_endp(usbd, usbb);
  if (!ep)
    return 0;
  return UE_ADDR(ep->addr);
}

uint8_t usb_status_type(usb_buf_t *usbb) {
  assert(usbb->flags & TF_CONTROL);
  return (!(usbb->flags & TF_INPUT) || !usbb->req_len ? TF_INPUT : 0);
}

uint8_t usb_interval(usb_device_t *usbd, usb_buf_t *usbb) {
  usb_endp_dsc_t *ep = usb_endp(usbd, usbb);
  if (!ep)
    return 0;
  return ep->interval;
}

void usb_process(usb_buf_t *usbb, void *data, transfer_flags_t flags) {
  WITH_SPIN_LOCK (&usbb->lock) {
    if (!flags) {
      if (usbb->flags & TF_INPUT)
        ringbuf_putnb(&usbb->buf, data, usbb->req_len);
      else
        usbb->buf.count = usbb->buf.size;
    } else {
      usbb->flags |= flags;
    }
    cv_signal(&usbb->cv);
  }
}

/* Alloc a device attached to port `prot`. */
static usb_device_t *usb_alloc_dev(uint8_t port) {
  usb_device_t *usbd = kmalloc(M_DEV, sizeof(usb_device_t), M_ZERO);
  /* Minimal possible packet size is 8. */
  usbd->dd.max_pkt_size = 8;
  usbd->port = port;
  return usbd;
}

static void usb_free_dev(usb_device_t *usbd) {
  if (usbd->endps)
    kfree(M_DEV, usbd->endps);
  kfree(M_DEV, usbd);
}

usb_buf_t *usb_alloc_buf(void *data, size_t size, transfer_flags_t flags,
                         uint16_t req_len) {
  usb_buf_t *usbb = kmalloc(M_DEV, sizeof(usb_buf_t), M_WAITOK);

  ringbuf_init(&usbb->buf, data, size);
  cv_init(&usbb->cv, "USB buffer ready");
  spin_init(&usbb->lock, 0);
  usbb->flags = flags;
  usbb->req_len = req_len;

  return usbb;
}

#define usb_alloc_buf_from_struct(sp, flags, req_len)                          \
  usb_alloc_buf((sp), sizeof(*(sp)), (flags), (req_len))

#define usb_alloc_empty_buf(flags) usb_alloc_buf(NULL, 1, (flags), 0)

void usb_free_buf(usb_buf_t *usbb) {
  kfree(M_DEV, usbb);
}

void usb_reuse_buf(usb_buf_t *usbb, void *data, size_t size,
                   transfer_flags_t flags, uint16_t req_len) {
  ringbuf_init(&usbb->buf, data, size);
  usbb->flags = flags;
  usbb->req_len = req_len;
}

void usb_reset_buf(usb_buf_t *usbb, uint16_t req_len) {
  ringbuf_reset(&usbb->buf);
  usbb->flags &= ~(TF_STALLED | TF_ERROR);
  usbb->req_len = req_len;
}

static inline uint16_t usb_wait_length(usb_buf_t *usbb) {
  return ((!(usbb->flags & TF_INPUT) && !usbb->req_len) ? 1 : usbb->req_len);
}

bool usb_transfer_error(usb_buf_t *usbb) {
  return usbb->flags & (TF_STALLED | TF_ERROR);
}

void usb_wait(usb_buf_t *usbb) {
  assert(spin_owned(&usbb->lock));

  ringbuf_t *rb = &usbb->buf;
  uint16_t wait_len = usb_wait_length(usbb);
  while (!usb_transfer_error(usbb) && rb->count < wait_len)
    cv_wait(&usbb->cv, &usbb->lock);
}

static int usb_control_transfer(usb_device_t *usbd, usb_buf_t *usbb,
                                usb_dev_req_t *req) {
  assert(usbb->flags & TF_CONTROL);

  usbhc_transfer(usbd, usbb, req);

  WITH_SPIN_LOCK (&usbb->lock) { usb_wait(usbb); }
  /* In control transfers, we consider a STALL condition to be an error
   * thereby, we assume that each standard request performed
   * is suported by a device. */
  if (usb_transfer_error(usbb))
    return 1;
  return 0;
}

int usb_set_idle(usb_device_t *usbd) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_WRITE_CLASS_INTERFACE,
    .req = UR_SET_IDLE,
    .idx = usbd->inum,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(usbd, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

int usb_set_boot_protocol(usb_device_t *usbd) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_WRITE_CLASS_INTERFACE,
    .req = UR_SET_PROTOCOL,
    .idx = usbd->inum,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(usbd, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

int usb_get_report(usb_device_t *usbd, void *buf, size_t size) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_READ_CLASS_INTERFACE,
    .req = UR_GET_REPORT,
    .val = UV_MAKE(1, 0),
    .idx = usbd->inum,
    .len = size,
  };
  usb_buf_t *usbb = usb_alloc_complete_buf(buf, size, TF_INPUT | TF_CONTROL);
  int error = usb_control_transfer(usbd, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

int usb_get_max_lun(usb_device_t *usbd, uint8_t *maxlun) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_READ_CLASS_INTERFACE,
    .req = UR_GET_MAX_LUN,
    .idx = usbd->inum,
    .len = 1,
  };
  usb_buf_t *usbb = usb_alloc_buf_from_struct(maxlun, TF_INPUT | TF_CONTROL, 1);
  int error = usb_control_transfer(usbd, usbb, &req);

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

int usb_bulk_only_reset(usb_device_t *usbd) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_WRITE_CLASS_INTERFACE,
    .req = UR_BULK_ONLY_RESET,
    .idx = usbd->inum,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(usbd, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

int usb_unhalt_endp(usb_device_t *usbd, uint8_t idx) {
  assert(idx < usbd->nendps);

  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_WRITE_ENDPOINT,
    .req = UR_CLEAR_FEATURE,
    .val = UF_ENDPOINT_HALT,
    .idx = UE_ADDR(usbd->endps[idx].addr),
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(usbd, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

void usb_interrupt_transfer(usb_device_t *usbd, usb_buf_t *usbb) {
  assert(usbb->flags & TF_INTERRUPT);
  usbhc_transfer(usbd, usbb, NULL);
}

int usb_poll(usb_device_t *usbd, usb_buf_t *usbb, uint8_t idx, void *buf,
             size_t size) {
  WITH_SPIN_LOCK (&usbb->lock) {
    for (;;) {
      usb_wait(usbb);

      if (!usb_transfer_error(usbb)) {
        ringbuf_getnb(&usbb->buf, buf, size);
        break;
      }

      if (usbb->flags & TF_STALLED && !(usbb->flags & TF_ERROR)) {
        usb_unhalt_endp(usbd, idx);
        usbb->flags &= ~(TF_ERROR | TF_STALLED);
        usb_interrupt_transfer(usbd, usbb);
      } else {
        return 1;
      }
    }
  }
  return 0;
}

void usb_bulk_transfer(usb_device_t *usbd, usb_buf_t *usbb) {
  assert(usbb->flags & TF_BULK);
  usbhc_transfer(usbd, usbb, NULL);
}

static int usb_set_addr(usb_device_t *usbd) {
  uint8_t addr = usb_max_addr++;
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_WRITE,
    .req = UR_SET_ADDRESS,
    .val = addr,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = 0;

  if (!(error = usb_control_transfer(usbd, usbb, &req))) {
    usbd->addr = addr;
    mdelay(2);
  }

  usb_free_buf(usbb);
  return error;
}

static int usb_get_dev_dsc(usb_device_t *usbd, usb_dev_dsc_t *dd) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_READ,
    .req = UR_GET_DESCRIPTOR,
    .val = UV_MAKE(UDESC_DEVICE, 0),
    .len = 8,
  };
  usb_buf_t *usbb = usb_alloc_buf_from_struct(dd, TF_INPUT | TF_CONTROL, 8);
  int error = 0;

  /* Read the first 8 bytes. */
  if ((error = usb_control_transfer(usbd, usbb, &req)))
    goto end;

  /* Get the whole descriptor. */
  req.len = dd->len;
  usb_reset_buf(usbb, req.len);
  error = usb_control_transfer(usbd, usbb, &req);

end:
  usb_free_buf(usbb);
  return error;
}

static int usb_get_str_lang_dsc(usb_device_t *usbd, usb_str_lang_t *langs) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_READ,
    .req = UR_GET_DESCRIPTOR,
    .val = UV_MAKE(UDESC_STRING, 0),
    .idx = USB_LANGUAGE_TABLE,
    .len = 2,
  };
  usb_buf_t *usbb = usb_alloc_buf_from_struct(langs, TF_INPUT | TF_CONTROL, 2);
  int error = 0;

  /* Obtain size of the lang table. */
  if ((error = usb_control_transfer(usbd, usbb, &req)))
    goto end;

  /* Read the whole lang table. */
  req.len = langs->len;
  usb_reset_buf(usbb, req.len);
  error = usb_control_transfer(usbd, usbb, &req);

end:
  usb_free_buf(usbb);
  return error;
}

static int usb_find_lang(usb_str_lang_t *langs, uint16_t lid) {
  uint8_t nlangs = (langs->len - 2) / 2;

  for (uint8_t i = 0; i < nlangs; i++)
    if (langs->data[i] == lid)
      return 0;

  return 1;
}

static int usb_english_support(usb_device_t *usbd) {
  usb_str_lang_t *langs = kmalloc(M_DEV, sizeof(usb_str_lang_t), M_WAITOK);
  int error = 0;

  if (!(error = usb_get_str_lang_dsc(usbd, langs)))
    error = usb_find_lang(langs, USENG_LID);

  if (!error)
    klog("device supports %s language", USENG_STR);
  else
    klog("device doesn't support %s language", USENG_STR);

  kfree(M_DEV, langs);
  return error;
}

static int usb_get_str_dsc(usb_device_t *usbd, uint8_t idx, usb_str_dsc_t *sd) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_READ,
    .req = UR_GET_DESCRIPTOR,
    .val = UV_MAKE(UDESC_STRING, idx),
    .idx = USENG_LID,
    .len = 2,
  };
  usb_buf_t *usbb = usb_alloc_buf_from_struct(sd, TF_INPUT | TF_CONTROL, 2);
  int error = 0;

  /* Obtain the size of the descriptor. */
  if ((error = usb_control_transfer(usbd, usbb, &req)))
    goto end;

  /* Read the whole descriptor. */
  req.len = sd->len;
  usb_reset_buf(usbb, req.len);
  error = usb_control_transfer(usbd, usbb, &req);

end:
  usb_free_buf(usbb);
  return error;
}

/* String descriptors use the UTF-16 encoding. */
static void usb_print_str_dsc(usb_str_dsc_t *sd, const char *msg) {
  char *buf = kmalloc(M_DEV, SDSC_DATASIZE + 1, M_WAITOK);
  uint8_t len = (sd->len - 2) / 2;
  uint8_t i = 0;

  for (; i < len; i++)
    buf[i] = sd->string[i];
  buf[i] = 0;

  klog("%s: %s", msg, buf);
}

static void usb_handle_str_dsc(usb_device_t *usbd, uint8_t idx,
                               const char *msg) {
  if (!idx)
    return;

  usb_str_dsc_t *sd = kmalloc(M_DEV, sizeof(usb_str_dsc_t), M_WAITOK);
  if (!usb_get_str_dsc(usbd, idx, sd))
    usb_print_str_dsc(sd, msg);

  kfree(M_DEV, sd);
}

#define CONFIG_SIZE 0x30

static int usb_get_config_dsc(usb_device_t *usbd, uint8_t num,
                              usb_config_dsc_t *cd) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_READ,
    .req = UR_GET_DESCRIPTOR,
    .val = UV_MAKE(UDESC_CONFIG, num),
    .len = 4,
  };
  usb_buf_t *usbb = usb_alloc_buf(cd, CONFIG_SIZE, TF_INPUT | TF_CONTROL, 4);
  int error = 0;

  /* Obtain the total size of the configuration. */
  if ((error = usb_control_transfer(usbd, usbb, &req)))
    goto end;

  /* Read the whole configuration. */
  req.len = cd->total_len;
  usb_reset_buf(usbb, req.len);
  error = usb_control_transfer(usbd, usbb, &req);

end:
  usb_free_buf(usbb);
  return error;
}

static int usb_set_configuration(usb_device_t *usbd, uint8_t val) {
  usb_dev_req_t req = (usb_dev_req_t){
    .type = UT_WRITE,
    .req = UR_SET_CONFIG,
    .val = val,
  };
  usb_buf_t *usbb = usb_alloc_empty_buf(TF_CONTROL);
  int error = usb_control_transfer(usbd, usbb, &req);

  usb_free_buf(usbb);
  return error;
}

static void usb_print_transfer_type(usb_endp_dsc_t *ed) {
  uint8_t tt = UE_TRANSFER_TYPE(ed->attributes);
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

static void usb_print_dev(usb_device_t *usbd, usb_config_dsc_t *cd,
                          usb_interface_dsc_t *id) {
  usb_dev_dsc_t *dd = &usbd->dd;

  klog("USB release: %04hx", dd->usb_release);
  klog("device class: %02hhx", dd->dev_class);
  klog("device subclass: %02hhx", dd->dev_subclass);
  klog("device protocol: %02hhx", dd->dev_protocol);
  klog("max config packet size: %02hhx", dd->max_pkt_size);
  klog("vendor ID: %04hx", dd->vendor_id);
  klog("product ID: %04hx", dd->product_id);
  klog("device release: %04hx", dd->dev_release);
  klog("number of configutarions: %02hhx", dd->nconfigurations);

  bool eng = !usb_english_support(usbd);

  if (eng) {
    usb_handle_str_dsc(usbd, dd->manufacturer_idx, "manufacturer");
    usb_handle_str_dsc(usbd, dd->product_idx, "product");
    usb_handle_str_dsc(usbd, dd->serialnum_idx, "serial number");
  }

  if (eng)
    usb_handle_str_dsc(usbd, cd->config_idx, "configuration");
  klog("number of interfaces: %hhu", cd->ninterfaces);
  klog("maximum power consumption: %hhu mA", cd->max_power * 2);

  if (eng)
    usb_handle_str_dsc(usbd, id->iidx, "interface");
  klog("number of endpoints: %hhu", id->nendpoints);

  for (int i = 0; i < usbd->nendps; i++) {
    usb_endp_dsc_t *ed = &usbd->endps[i];
    klog("endpoint %d:", i);
    usb_print_transfer_type(ed);
    klog("address: %hhu", UE_ADDR(ed->addr));
    if (UE_DIR(ed->addr))
      klog("direction: input");
    else
      klog("direction: output");
    klog("max packet size: %hu", ed->max_pkt_size);
    if (ed->interval)
      klog("interval: %hhu", ed->interval);
    else
      klog("no polling required");
  }
}

static usb_endp_dsc_t *usb_endp_dsc_addr(usb_interface_dsc_t *id) {
  if (id->iclass == UICLASS_HID) {
    usb_hid_dsc_t *hd = (usb_hid_dsc_t *)(id + 1);
    return (void *)hd + hd->len;
  }
  return (usb_endp_dsc_t *)(id + 1);
}

static int usb_configure(usb_device_t *usbd) {
  usb_config_dsc_t *cd = kmalloc(M_DEV, CONFIG_SIZE, M_WAITOK);
  int error = 0;

  if ((error = usb_get_config_dsc(usbd, 0, cd)))
    goto end;

  usb_interface_dsc_t *id = (usb_interface_dsc_t *)(cd + 1);

  if (!(usbd->dd.dev_class)) {
    usbd->dd.dev_class = id->iclass;
    usbd->dd.dev_subclass = id->isubclass;
    usbd->dd.dev_protocol = id->iprotocol;
  }
  usbd->inum = id->inum;

  usbd->nendps = id->nendpoints;
  usbd->endps = kmalloc(M_DEV, sizeof(usb_endp_dsc_t) * usbd->nendps, M_WAITOK);

  usb_endp_dsc_t *ed = usb_endp_dsc_addr(id);
  for (int i = 0; i < usbd->nendps; i++)
    memcpy(&usbd->endps[i], &ed[i], sizeof(usb_endp_dsc_t));

  if ((error = usb_set_configuration(usbd, cd->config_val)))
    goto end;

  usb_print_dev(usbd, cd, id);

end:
  kfree(M_DEV, cd);
  return error;
}

static int usb_identify(usb_device_t *usbd) {
  if (usb_get_dev_dsc(usbd, &usbd->dd))
    return 1;

  usbhc_reset_port(usbd->port);

  if (usb_set_addr(usbd))
    return 1;

  return 0;
}

void usb_enumerate(device_t *dev) {
  uint8_t nports = usbhc_number_of_ports();

  for (uint8_t pn = 0; pn < nports; pn++) {
    if (!usbhc_device_present(pn)) {
      klog("no device attached to port %hhu", pn);
      continue;
    }
    klog("device attached to port %hhu", pn);

    usb_device_t *usbd = usb_alloc_dev(pn);
    if (usb_identify(usbd)) {
      klog("failed to identify the device at port %hhu", pn);
      goto bad;
    }

    if (usb_configure(usbd)) {
      klog("failed to configure the device at port %hhu", pn);
      goto bad;
    }

    device_t *d = device_add_child(dev, pn);
    d->instance = usbd;
    continue;

  bad:
    usb_free_dev(usbd);
    continue;
  }
}
