#define KL_LOG KL_USB
#include <sys/klog.h>
#include <sys/vnode.h>
#include <sys/devclass.h>
#include <sys/devfs.h>
#include <sys/device.h>
#include <sys/ringbuf.h>
#include <dev/usb.h>

typedef struct hidm_report {
  uint8_t buttons;
  uint8_t x;
  uint8_t y;
} __packed hidm_report_t;

#define HIDM_REPORT_NUM 64
#define HIDM_BUFFER_SIZE (sizeof(hidm_report_t) * HIDM_REPORT_NUM)

typedef struct hidm_state {
  usb_buf_t *buf;
} hidm_state_t;

static int hidm_read(vnode_t *v, uio_t *uio) {
  device_t *dev = devfs_node_data(v);
  hidm_state_t *hidm = dev->state;

  uio->uio_offset = 0;

  hidm_report_t report;
  int error = usb_buf_read(hidm->buf, &report, 0);
  if (!error)
    return uiomove_frombuf(&report, sizeof(hidm_report_t), uio);

  usb_error_t uerr = usb_buf_error(hidm->buf);
  if (uerr != USB_ERR_STALLED)
    return error;

  if ((error = usb_unhalt_endp(dev, USB_TFR_INTERRUPT, USB_DIR_INPUT)))
    return error;

  usb_interrupt_transfer(dev, hidm->buf, USB_DIR_INPUT, sizeof(hidm_report_t));

  return error;
}

static vnodeops_t hidm_ops = {
  .v_read = hidm_read,
};

static int hidm_probe(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);

  /* We're looking for a HID mouse wwhich supports the boot interface. */
  if (udev->class_code != UICLASS_HID ||
      udev->subclass_code != UISUBCLASS_BOOT ||
      udev->protocol_code != UIPROTO_MOUSE)
    return 0;

  return 1;
}

static int hidm_attach(device_t *dev) {
  hidm_state_t *hidm = dev->state;

  if (usb_hid_set_idle(dev))
    return ENXIO;

  if (usb_hid_set_boot_protocol(dev))
    return ENXIO;

  /* Prepare a report buffer. */
  hidm->buf = usb_buf_alloc(HIDM_BUFFER_SIZE);

  usb_interrupt_transfer(dev, hidm->buf, USB_DIR_INPUT, sizeof(hidm_report_t));

  /* Prepare /dev/hidm interface. */
  devfs_makedev(NULL, "hidm", &hidm_ops, dev, NULL);

  return 0;
}

static driver_t hidm_driver = {
  .desc = "HID mouse driver",
  .size = sizeof(hidm_state_t),
  .pass = SECOND_PASS,
  .probe = hidm_probe,
  .attach = hidm_attach,
};

DEVCLASS_ENTRY(usb, hidm_driver);
