/*
 * The following routines define an interface implemented by each USB host
 * controller. This interface is meant to be used by the USB bus methods
 * to service requests issued by USB device drivers.
 */
#ifndef _DEV_USBHC_H_
#define _DEV_USBHC_H_

#include <dev/usb.h>

typedef uint8_t (*usbhc_number_of_ports_t)(device_t *hcdev);
typedef bool (*usbhc_device_present_t)(device_t *hcdev, uint8_t port);
typedef usb_speed_t (*usbhc_device_speed_t)(device_t *hcdev, uint8_t port);
typedef void (*usbhc_reset_port_t)(device_t *hcdev, uint8_t port);
typedef void (*usbhc_control_transfer_t)(device_t *hcdev, device_t *dev,
                                         usb_buf_t *buf, usb_dev_req_t *req,
                                         usb_direction_t status_dir);
typedef void (*usbhc_data_transfer_t)(device_t *hcdev, device_t *dev,
                                      usb_buf_t *buf);

typedef struct usbhc_methods {
  usbhc_number_of_ports_t number_of_ports;
  usbhc_device_present_t device_present;
  usbhc_device_speed_t device_speed;
  usbhc_reset_port_t reset_port;
  usbhc_control_transfer_t control_transfer;
  usbhc_data_transfer_t data_transfer;
} usbhc_methods_t;

static inline usbhc_methods_t *usbhc_methods(device_t *dev) {
  return (usbhc_methods_t *)dev->driver->interfaces[DIF_USBHC];
}

/*! \brief Returns the number of root hub ports.
 *
 * This function will be called during enumeration of ports
 * controlled by the specified host controller.
 *
 * \param hcdev host controller device
 */
static inline uint8_t usbhc_number_of_ports(device_t *hcdev) {
  return usbhc_methods(hcdev)->number_of_ports(hcdev);
}

/* TODO: remove the `parent` reference in the following construct after
 * fixing method dispathing. */
#define USBHC_METHOD_PROVIDER(dev, method)                                     \
  (device_method_provider((dev), DIF_USBHC,                                    \
                          offsetof(struct usbhc_methods, method))              \
     ->parent)

/*! \brief Checks whether any device is attached to the specified
 * root hub port.
 *
 * This function will be called during enumeration of ports
 * controlled by specified host controller.
 *
 * \param hcdev host controller device
 * \param port number of the root hub port to examine
 */
static inline bool usbhc_device_present(device_t *hcdev, uint8_t port) {
  return usbhc_methods(hcdev)->device_present(hcdev, port);
}

/*! \brief Returns speed of the device attached to the specified
 * root hub port.
 *
 * This function will be called during enumeration of ports
 * controlled by specified host controller.
 *
 * \param hcdev host controller device
 * \param port number of the root hub port to examine
 */
static inline usb_speed_t usbhc_device_speed(device_t *hcdev, uint8_t port) {
  return usbhc_methods(hcdev)->device_speed(hcdev, port);
}

/*! \brief Resets the specified root hub port.
 *
 * This function is essential to bring the attached device to the
 * default state.
 *
 * \param hcdev host controller device
 * \param port number of the root hub port to examine
 */
static inline void usbhc_reset_port(device_t *hcdev, uint8_t port) {
  usbhc_methods(hcdev)->reset_port(hcdev, port);
}

/*! \brief Schedules a control transfer between the host and
 * the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB device
 * \param buf USB buffer used for the transfer
 * \param req USB device request
 * \param status_dir direction for the STATUS stage of the transfer
 */
static inline void usbhc_control_transfer(device_t *dev, usb_buf_t *buf,
                                          usb_dev_req_t *req,
                                          usb_direction_t status_dir) {
  device_t *hcdev = USBHC_METHOD_PROVIDER(dev, control_transfer);
  usbhc_methods(hcdev)->control_transfer(hcdev, dev, buf, req, status_dir);
}

/*! \brief Schedules a data stage only transfer between the host
 * and the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB device
 * \param buf USB buffer used for the transfer
 */
static inline void usbhc_data_transfer(device_t *dev, usb_buf_t *buf) {
  device_t *hcdev = USBHC_METHOD_PROVIDER(dev, data_transfer);
  usbhc_methods(hcdev)->data_transfer(hcdev, dev, buf);
}

#endif /* _DEV_USBHC_H_ */
