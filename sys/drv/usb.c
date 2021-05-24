#include <dev/usb.h>

usb_buf_t *usb_buf_alloc(void) {
  /* Not implemented! */
  return NULL;
}

void usb_buf_free(usb_buf_t *buf) {
  /* Not implemented! */
}

int usb_buf_wait(usb_buf_t *buf) {
  /* Not implemented! */
  return 0;
}

int usb_unhalt_endpt(device_t *dev, usb_transfer_t transfer,
                     usb_direction_t dir) {
  /* Not implemented! */
  return 0;
}

int usb_hid_set_idle(device_t *dev) {
  /* Not implemented! */
  return 0;
}

int usb_hid_set_boot_protocol(device_t *dev) {
  /* Not implemented! */
  return 0;
}
