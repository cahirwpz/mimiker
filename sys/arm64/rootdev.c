#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/devclass.h>
#include <sys/sysinit.h>

static void rootdev_init(void) {
}

SYSINIT_ADD(rootdev, rootdev_init, DEPS("mount_fs", "ithread"));
DEVCLASS_CREATE(root);
