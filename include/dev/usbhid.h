/*
 * Based on FreeBSD `sys/dev/usb/usbhid.h`.
 */
#ifndef _DEV_HID_H_
#define _DEV_HID_H_

#define UDESC_REPORT 0x01
#define UR_GET_REPORT 0x01
#define UR_SET_REPORT 0x09
#define UR_SET_IDLE 0x0a
#define UR_SET_PROTOCOL 0x0b

#define UHID_INPUT_REPORT 0x01
#define UHID_OUTPUT_REPORT 0x02
#define UHID_FEATURE_REPORT 0x03

/* Keyboard LEDs in boot output report. */
#define UHID_KBDLED_NUMLOCK 0x01
#define UHID_KBDLED_CAPSLOCK 0x02
#define UHID_KBDLED_SCROLLLOCK 0x04

typedef struct usb_hid_dsc {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdHID;
  uint8_t bCountryCode;
  uint8_t bNumDescriptors;
  struct {
    uint8_t bDescriptorType;
    uint16_t wDescriptorLength;
  } descrs[1];
} __packed usb_hid_dsc_t;

#endif /* _DEV_HID_H_ */
