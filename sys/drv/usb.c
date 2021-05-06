#include <dev/usb.h>

bool usb_buf_periodic(usb_buf_t *buf) {
  /* Not yet implemented! */
  return true;
}

usb_direction_t usb_buf_status_direction(usb_buf_t *buf) {
  /* Not yet implemented! */
  return 0;
}

void usb_buf_copy_data(usb_buf_t *buf, void *dst) {
  /* Not yet implemented! */
}

void usb_process(usb_buf_t *buf, void *data, usb_error_t error) {
  /* Not yet implemented! */
}

void usb_init(device_t *dev) {
  /* Not yet implemented! */
}

int usb_enumerate(device_t *dev) {
  /* Not yet implemented! */
  return 0;
}
