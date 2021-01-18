#ifndef _HID_H_
#define _HID_H_

#define UDESC_REPORT 0x01
#define UR_GET_REPORT 0x01
#define UR_SET_IDLE 0x0a
#define UR_SET_PROTOCOL 0x0b

struct usb_hid_dsc {
  uint8_t len;
  uint8_t type;
  uint16_t release;
  uint8_t country_code;
  uint8_t ndescriptors;
  struct {
    uint8_t type;
    uint8_t len;
  } descrs[1];
} __packed;

typedef struct usb_hid_dsc usb_hid_dsc_t;

#endif /* _HID_H_ */
