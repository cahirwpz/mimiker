#include <sys/vnode.h>
#include <sys/devclass.h>
#include <sys/devfs.h>
#include <sys/device.h>
#include <sys/ringbuf.h>
#include <dev/usb.h>

#define HIDKBD_NKEYCODES 6

/* HID keyboard input report */
typedef struct hidkbd_in_report {
  uint8_t modifier_keys;
  uint8_t reserved;
  uint8_t keycodes[HIDKBD_NKEYCODES];
} __packed hidkbd_in_report_t;

#define HIDKBD_IN_REPORT_NUM 32
#define HIDKBD_BUFFER_SIZE (sizeof(hidkbd_in_report_t) * HIDKBD_IN_REPORT_NUM)

typedef struct hidkbd_state {
  usb_buf_t *buf;
} hidkbd_state_t;

static int hidkbd_read(vnode_t *v, uio_t *uio) {
  device_t *dev = devfs_node_data(v);
  hidkbd_state_t *hidkbd = dev->state;

  uio->uio_offset = 0;

  hidkbd_in_report_t report;
  int error = usb_buf_read(hidkbd->buf, &report, 0);
  if (!error)
    return uiomove_frombuf(&report, sizeof(hidkbd_in_report_t), uio);

  usb_error_t uerr = usb_buf_error(hidkbd->buf);
  if (uerr != USB_ERR_STALLED)
    return error;

  if ((error = usb_unhalt_endp(dev, USB_TFR_INTERRUPT, USB_DIR_INPUT)))
    return error;

  usb_interrupt_transfer(dev, hidkbd->buf, USB_DIR_INPUT,
                         sizeof(hidkbd_in_report_t));

  return error;
}

static vnodeops_t hidkbd_ops = {
  .v_read = hidkbd_read,
};

static int hidkbd_probe(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  if (!udev)
    return 0;

  /* We're looking for a HID keyboard with a boot interface. */
  if (udev->class_code != UICLASS_HID ||
      udev->subclass_code != UISUBCLASS_BOOT ||
      udev->protocol_code != UIPROTO_BOOT_KEYBOARD)
    return 0;

  return 1;
}

static int hidkbd_attach(device_t *dev) {
  hidkbd_state_t *hidkbd = dev->state;

  if (usb_hid_set_idle(dev))
    return ENXIO;

  if (usb_hid_set_boot_protocol(dev))
    return ENXIO;

  /* Prepare a report buffer. */
  hidkbd->buf = usb_buf_alloc(HIDKBD_BUFFER_SIZE);

  usb_interrupt_transfer(dev, hidkbd->buf, USB_DIR_INPUT,
                         sizeof(hidkbd_in_report_t));

  /* Prepare /dev/hidkbd interface. */
  devfs_makedev(NULL, "hidkbd", &hidkbd_ops, dev, NULL);

  return 0;
}

static driver_t hidkbd_driver = {
  .desc = "HID keyboard driver",
  .size = sizeof(hidkbd_state_t),
  .pass = SECOND_PASS,
  .probe = hidkbd_probe,
  .attach = hidkbd_attach,
};

DEVCLASS_ENTRY(usb, hidkbd_driver);
