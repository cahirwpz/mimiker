/*
 * Inspired by FreeBSD's code in `sys/dev/evdev/`.
 */

#include <dev/evdev.h>

/* NOTE: FTTB, we only support the following mouse buttons. */
static uint16_t evdev_mouse_button_codes[] = {
  BTN_LEFT,
  BTN_MIDDLE,
  BTN_RIGHT,
};

uint16_t evdev_hid2btn(uint8_t btnum) {
  return evdev_mouse_button_codes[btnum];
}

void evdev_support_all_mouse_buttons(evdev_dev_t *evdev) {
  size_t nitems = sizeof(evdev_mouse_button_codes) / sizeof(uint16_t);
  evdev_support_all_keys(evdev, evdev_mouse_button_codes, nitems);
}
