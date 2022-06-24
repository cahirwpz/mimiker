#ifndef _DEV_FDT_DEV_H_
#define _DEV_FDT_DEV_H_

#include <sys/device.h>

/*
 * Enumerate simiplebus devices.
 *
 * Arguments:
 *  - `rootdev`: root bus device
 *  -  `rm`: soc memory resource meneger to populate
 *
 * Returns:
 *  - 0: success
 *  - != 0: errno code identifying the occured error
 */
int simplebus_enumerate(device_t *rootdev, rman_t *rm);

/*
 * Add a device that is not a child of the SOC device tree node
 * based on the FDT.
 *
 * Arguments:
 *  - `rootdev`: root bus device
 *  - `path`: absolute FDT device path
 *
 * Returns:
 *  - 0: success
 *  - != 0: errno code identifying the occured error
 */
int FDT_add_child(device_t *rootdev, const char *path);

#endif /* !_DEV_FDT_DEV_H_ */
