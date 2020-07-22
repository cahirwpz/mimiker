#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/devclass.h>

static bus_driver_t rootdev_driver;

DEVCLASS_CREATE(root);

static device_t rootdev = (device_t){
  .children = TAILQ_HEAD_INITIALIZER(rootdev.children),
    .driver = (driver_t *)&rootdev_driver,
    .state = NULL,
    .devclass = &DEVCLASS(root),
};

void init_devices(void) {
}

/* 
 * XXX(pj): linker set needs at least one element otherwise ld will be very sad
 */
DEVCLASS_ENTRY(root, rootdev);
