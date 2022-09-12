/*
 * USB HID mouse driver.
 *
 * Relevant documents:
 *
 * - Device Class Definition for Human Interface Devices (HID) Version 1.11,
 *   5/27/01:
 *     https://www.usb.org/sites/default/files/hid1_11.pdf
 *
 * NOTE: for simplicity, we rely on the boot protocol.
 */
#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/devclass.h>
#include <sys/device.h>
#include <sys/sched.h>
#include <dev/evdev.h>
#include <dev/usb.h>

#define HIDMS_NBUTTONS 3

/* HID mouse boot input report. */
typedef struct hidms_boot_in_rep {
  uint8_t buttons; /* button status */
  int8_t x;        /* relative X movement */
  int8_t y;        /* relative Y movement */
} __packed hidms_boot_in_rep_t;

/* HID mouse software context. */
typedef struct hidms_state {
  thread_t *thread;     /* input packets gathering thread */
  evdev_dev_t *evdev;   /* evdev device structure */
  uint8_t prev_buttons; /* the most recent buttons state */
} hidms_state_t;

/*
 * Evdev handling functions.
 */

static void hidms_init_evdev(device_t *dev) {
  hidms_state_t *hidms = dev->state;
  evdev_dev_t *evdev = evdev_alloc();

  /* Set the basic evdev parameters. */
  usb_device_t *udev = usb_device_of(dev);
  evdev_set_name(evdev, "HID mouse");
  evdev_set_id(evdev, BUS_USB, udev->vendor_id, udev->product_id, 0);

  /* `EV_SYN` is required for every evdev device. */
  evdev_support_event(evdev, EV_SYN);

  /* Mouse buttons. */
  evdev_support_event(evdev, EV_KEY);
  evdev_support_all_mouse_buttons(evdev);

  /* Mouse movement. */
  evdev_support_event(evdev, EV_REL);
  evdev_support_rel(evdev, REL_X);
  evdev_support_rel(evdev, REL_Y);

  /* Bind `evdev` to `hidms` for future references. */
  hidms->evdev = evdev;

  /* Finally, register the device in the file system. */
  evdev_register(evdev);
}

/*
 * Report processing functions.
 */

static bool hidms_process_buttons(hidms_state_t *hidms, uint8_t buttons) {
  uint8_t prev_buttons = hidms->prev_buttons;
  bool sync = false;

  for (size_t i = 0; i < HIDMS_NBUTTONS; i++) {
    uint8_t prev = prev_buttons & (1 << i);
    uint8_t cur = buttons & (1 << i);

    if (cur == prev)
      continue;

    uint16_t keycode = evdev_hid2btn(i);
    evdev_push_key(hidms->evdev, keycode, cur);
    sync = true;
  }

  return sync;
}

static void hidms_process_in_report(hidms_state_t *hidms,
                                    hidms_boot_in_rep_t *report) {
  /* Handle buttons. */
  bool btns = hidms_process_buttons(hidms, report->buttons);

  /* Handle the X-axis. */
  int8_t x = report->x;
  if (x)
    evdev_push_event(hidms->evdev, EV_REL, REL_X, x);

  /* Handle the Y-axis. */
  int8_t y = report->y;
  if (y)
    evdev_push_event(hidms->evdev, EV_REL, REL_Y, y);

  if (btns || x || y)
    evdev_sync(hidms->evdev);
}

/* A thread which gathers input reports, processes and converts them
 * to evdev-compatible events and then hands them over to the evdev layer. */
static void hidms_thread(void *arg) {
  device_t *dev = arg;
  hidms_state_t *hidms = dev->state;
  hidms_boot_in_rep_t report;

  /* Obtain a USB buf. */
  usb_buf_t *buf = usb_buf_alloc();

  /* Register an interrupt input transfer. */
  usb_data_transfer(dev, buf, &report, sizeof(hidms_boot_in_rep_t),
                    USB_TFR_INTERRUPT, USB_DIR_INPUT);

  for (;;) {
    /* Wait for the data to arrive. */
    int error = usb_buf_wait(buf);
    if (!error) {
      hidms_process_in_report(hidms, &report);
      hidms->prev_buttons = report.buttons;
      continue;
    }

    /* Recover and reenable the transfer. */
    if ((error = usb_unhalt_endpt(dev, USB_TFR_INTERRUPT, USB_DIR_INPUT))) {
      /* TODO: we should have a way to unregister an evdev device. */
      panic("Unable to unhalt an endpoint");
    }

    usb_data_transfer(dev, buf, &report, sizeof(hidms_boot_in_rep_t),
                      USB_TFR_INTERRUPT, USB_DIR_INPUT);
  }
}

/*
 * Driver interface implementation.
 */

static int hidms_probe(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  if (!udev)
    return 0;

  /* We're looking for a HID mouse wwhich supports the boot interface. */
  if (udev->class_code != UICLASS_HID ||
      udev->subclass_code != UISUBCLASS_BOOT ||
      udev->protocol_code != UIPROTO_MOUSE)
    return 0;

  return 1;
}

static int hidms_attach(device_t *dev) {
  hidms_state_t *hidms = dev->state;

  if (usb_hid_set_idle(dev))
    return ENXIO;

  /* We rely on the boot protocol report layout. */
  if (usb_hid_set_boot_protocol(dev))
    return ENXIO;

  hidms->thread =
    thread_create("hidms", hidms_thread, dev, prio_ithread(PRIO_ITHRD_QTY - 1));

  hidms_init_evdev(dev);

  sched_add(hidms->thread);

  return 0;
}

static driver_t hidms_driver = {
  .desc = "HID mouse driver",
  .size = sizeof(hidms_state_t),
  .pass = SECOND_PASS,
  .probe = hidms_probe,
  .attach = hidms_attach,
};

DEVCLASS_ENTRY(usb, hidms_driver);
