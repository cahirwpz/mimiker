#include <sys/vnode.h>
#include <sys/devclass.h>
#include <sys/devfs.h>
#include <sys/device.h>
#include <sys/ringbuf.h>
#include <dev/usb.h>

#define HIDKBD_NKEYCODES 6

/* HID keyboard input report */
struct hidkbd_inr {
  uint8_t modifier_keys;
  uint8_t reserved;
  uint8_t keycodes[HIDKBD_NKEYCODES];
} __packed;

typedef struct hidkbd_inr hidkbd_inr_t;

#define HIDKBD_INR_NUM 32
#define HIDKBD_BUFFER_SIZE (sizeof(hidkbd_inr_t) * HIDKBD_INR_NUM)

typedef struct hidkbd_state {
  usb_buf_t *usbb;
} hidkbd_state_t;

static int hidkbd_probe(device_t *dev) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_descriptor_t *dd = &usbd->dd;

  /* We're looking for a HID keyboard with a boot interface. */
  if (dd->bDeviceClass != UICLASS_HID ||
      dd->bDeviceSubClass != UISUBCLASS_BOOT ||
      dd->bDeviceProtocol != UIPROTO_BOOT_KEYBOARD)
    return 0;

  /* Check if supplied endpoint is an input interrupt endpoint. */
  if (usb_endp_type(usbd, 0) != UE_INTERRUPT || !usb_endp_dir(usbd, 0))
    return 0;

  return 1;
}

static int hidkbd_read(vnode_t *v, uio_t *uio, __unused int ioflags) {
  device_t *dev = devfs_node_data(v);
  hidkbd_state_t *hidkbd = dev->state;

  uio->uio_offset = 0;

  hidkbd_inr_t report;
  int error = usb_poll(dev, hidkbd->usbb, 0, &report, sizeof(hidkbd_inr_t));
  if (error)
    return 1;

  return uiomove_frombuf(&report, sizeof(hidkbd_inr_t), uio);
}

static vnodeops_t hidkbd_ops = {
  .v_read = hidkbd_read,
};

static int hidkbd_attach(device_t *dev) {
  hidkbd_state_t *hidkbd = dev->state;

  if (usb_set_idle(dev))
    return 1;

  if (usb_set_boot_protocol(dev))
    return 1;

  /* Prepare a report buffer. */
  void *buf = kmalloc(M_DEV, HIDKBD_BUFFER_SIZE, M_WAITOK);
  hidkbd->usbb = usb_alloc_buf(buf, HIDKBD_BUFFER_SIZE, TF_INPUT | TF_INTERRUPT,
                               sizeof(hidkbd_inr_t));

  usb_interrupt_transfer(dev, hidkbd->usbb);

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

DEVCLASS_DECLARE(usbhc);
DEVCLASS_ENTRY(usbhc, hidkbd_driver);
