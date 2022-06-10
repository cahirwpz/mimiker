/*
 * Based on FreeBSD `sys/dev/usb/usbhid.h`.
 */
#ifndef _DEV_HID_H_
#define _DEV_HID_H_

#define UDESC_REPORT 0x01
#define UR_GET_REPORT 0x01
#define UR_SET_IDLE 0x0a
#define UR_SET_PROTOCOL 0x0b

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
