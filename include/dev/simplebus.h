#ifndef _DEV_SIMPLEBUS_H_
#define _DEV_SIMPLEBUS_H_

typedef struct device device_t;

/*
 * Add a simplebus-attached device.
 *
 * Arguments:
 *  - `bus`: parent bus of the device
 *  - `path`: absolute FDT device path
 *  - `unit`: unit number
 *  - `pic`: programmable interrupt controller of the device
 *  - `devp`: if not `NULL`, dst for the pointer of the allocated device
 *
 * Returns:
 *  - 0: success
 *  - != 0: errno code identifying the occured error
 */
int simplebus_add_child(device_t *bus, const char *path, int unit,
                        device_t *pic, device_t **devp);

#endif /* !_DEV_SIMPLEBUS_H_ */
