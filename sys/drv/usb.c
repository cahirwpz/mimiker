#include <dev/usb.h>

bool usb_buf_periodic(usb_buf_t *buf) {
  /* Not yet implemented! */
  return true;
}

void usb_buf_process(usb_buf_t *buf, void *data, usb_error_t error) {
  /* Not yet implemented! */
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
