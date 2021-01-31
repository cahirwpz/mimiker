/*
 * Based on FreeBSD `usbhid.h`.
 */
#ifndef _HID_H_
#define _HID_H_

#define UDESC_REPORT 0x01
#define UR_GET_REPORT 0x01
#define UR_SET_IDLE 0x0a
#define UR_SET_PROTOCOL 0x0b

typedef struct usb_hid_descriptor {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdHID;
  uint8_t bCountryCode;
  uint8_t bNumDescriptors;
  struct {
    uint8_t bDescriptorType;
    uint16_t wDescriptorLength;
  } descrs[1];
} __packed usb_hid_descriptor_t;

#endif /* _HID_H_ */
