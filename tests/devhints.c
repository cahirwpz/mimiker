#include <stdc.h>
#include <ktest.h>
#include <klog.h>
#include <fdt.h>
#include <devhints.h>
#include <devclass.h>

extern device_t *gt_pci;

extern uint8_t __malta_dtb_start[];
const void *fdt = (const void *)__malta_dtb_start;

static int test_fdt_sandbox(void) {
  devclass_t *dc;
  driver_t **drv;

  dc = devclass_find("pci");
  assert(gt_pci != NULL);

  DEVCLASS_FOREACH(drv, dc) {
    bus_enumerate_hinted_children(gt_pci);
  }
  return KTEST_SUCCESS;
}

KTEST_ADD(fdt_sandbox, test_fdt_sandbox, 0);
