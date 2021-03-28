#define KL_LOG KL_USB
#include <sys/klog.h>
#include <sys/vnode.h>
#include <sys/devclass.h>
#include <sys/devfs.h>
#include <sys/device.h>
#include <sys/ringbuf.h>
#include <dev/usb.h>

struct hidm_report {
  uint8_t buttons;
  uint8_t x;
  uint8_t y;
} __packed;

typedef struct hidm_report hidm_report_t;

#define HIDM_REPORT_NUM 64
#define HIDM_BUFFER_SIZE (sizeof(hidm_report_t) * HIDM_REPORT_NUM)

typedef struct hidm_state {
  usb_buf_t *usbb;
} hidm_state_t;

static int hidm_probe(device_t *dev) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_descriptor_t *dd = &usbd->dd;

  /* We're looking for a HID mouse with the boot interface. */
  if (dd->bDeviceClass != UICLASS_HID ||
      dd->bDeviceSubClass != UISUBCLASS_BOOT ||
      dd->bDeviceProtocol != UIPROTO_MOUSE)
    return 0;

  /* Check if supplied endpoint is an input interrupt endpoint. */
  if (usb_endp_type(usbd, 0) != UE_INTERRUPT || !usb_endp_dir(usbd, 0))
    return 0;

  return 1;
}

static int hidm_read(vnode_t *v, uio_t *uio, __unused int ioflags) {
  device_t *dev = devfs_node_data(v);
  hidm_state_t *hidm = dev->state;

  uio->uio_offset = 0;

  hidm_report_t report;
  int error = usb_poll(dev, hidm->usbb, 0, &report, sizeof(hidm_report_t));
  if (error)
    return 1;

  return uiomove_frombuf(&report, sizeof(hidm_report_t), uio);
}

static vnodeops_t hidm_ops = {
  .v_read = hidm_read,
};

static int hidm_attach(device_t *dev) {
  hidm_state_t *hidm = dev->state;

  if (usb_set_idle(dev))
    return 1;

  if (usb_set_boot_protocol(dev))
    return 1;

  /* Prepare a report buffer. */
  void *buf = kmalloc(M_DEV, HIDM_BUFFER_SIZE, M_WAITOK);
  hidm->usbb = usb_alloc_buf(buf, HIDM_BUFFER_SIZE, TF_INPUT | TF_INTERRUPT,
                             sizeof(hidm_report_t));

  usb_interrupt_transfer(dev, hidm->usbb);

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

DEVCLASS_DECLARE(usbhc);
DEVCLASS_ENTRY(usbhc, hidm_driver);
