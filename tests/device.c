#include <ktest.h>
#include <malloc.h>
#include <device.h>

#include <stdc.h>
#include <vnode.h>
#include <devfs.h>
#include <klog.h>
#include <condvar.h>
#include <ringbuf.h>
#include <pci.h>
#include <dev/isareg.h>
#include <dev/ns16550reg.h>
#include <interrupt.h>
#include <sysinit.h>

#if 0
extern device_t *gt_pci;

static int test_operate_on_devprops_attrs(void) {
  devprop_attr_t *attr;
  devprop_res_key_t key = FDT_PATH;
  devprop_attr_val_t value;
  value.str_value = "attribute_value";
  set_device_prop_attr(gt_pci, key, &value);
  attr = get_device_prop_attr(gt_pci, key);

  assert(!strcmp(attr->value->str_value, "attribute_value"));
  return KTEST_SUCCESS;
}

static int test_get_non_existing_deprop_attr_res_null(void) {
  devprop_attr_t *attr;
  devprop_res_key_t key = FDT_PATH;
  attr = get_device_prop_attr(gt_pci, key);

  assert(attr == NULL);
  return KTEST_SUCCESS;
}

static int test_override_exisiting_devprop_attr(void) {
  devprop_attr_t *attr;
  devprop_res_key_t key = FDT_PATH;
  devprop_attr_val_t value;

  value.str_value = "attribute_value";
  set_device_prop_attr(gt_pci, key, &value);
  value.str_value = "overridden_value";
  set_device_prop_attr(gt_pci, key, &value);
  attr = get_device_prop_attr(gt_pci, key);

  assert(!strcmp(attr->value->str_value, "overridden_value"));
  return KTEST_SUCCESS;
}

KTEST_ADD(operate_on_devprops_attrs, test_operate_on_devprops_attrs, 0);
KTEST_ADD(get_non_existing_deprop_attr_res_null,
          test_get_non_existing_deprop_attr_res_null, 0);
KTEST_ADD(override_exisiting_devprop_attr, test_override_exisiting_devprop_attr,
          0);
#endif
