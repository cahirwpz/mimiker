#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/devclass.h>

driver_t rootdev_driver;

DEVCLASS_CREATE(root);

/* We need at least one device in each devclass for the kernel to link
 * successfully. FTTB, let's just put `rootdev_driver` in `root` dc. */
DEVCLASS_ENTRY(root, rootdev_driver);
