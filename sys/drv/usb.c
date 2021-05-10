#include <dev/usb.h>

void usb_buf_copyout(usb_buf_t *buf, void *dst, size_t size) {
  /* Not yet implemented! */
}

usb_direction_t usb_buf_status_dir(usb_buf_t *buf) {
  /* Not yet implemented! */
  return 0;
}
usb_direction_t usb_buf_dir(usb_buf_t *buf) {
  /* Not yet implemented! */
  return 0;
}

uint16_t usb_buf_transfer_size(usb_buf_t *buf) {
  /* Not yet implemented! */
  return 0;
}

void usb_buf_process(usb_buf_t *buf, void *data, usb_error_t error) {
  /* Not yet implemented! */
}

bool usb_buf_periodic(usb_buf_t *buf) {
  /* Not yet implemented! */
  return true;
}

usb_direction_t usb_status_dir(usb_direction_t dir, uint16_t transfer_size) {
  /* Not yet implemented! */
  return 0;
}

void usb_init(device_t *dev) {
  /* Not yet implemented! */
}

int usb_enumerate(device_t *dev) {
  /* Not yet implemented! */
  return 0;
}
