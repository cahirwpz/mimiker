#include <ktest.h>
#include <device.h>

extern device_t *gt_pci;

static int test_operate_on_devprops_attrs(void) {
  devprop_attr_t *attr;
  devprop_res_key_t key = FDT_PATH;
  set_device_prop_attr(gt_pci, key, "attribute_value");
  attr = get_device_prop_attr(gt_pci, key);

  assert(!strcmp(attr->value.str_value, "attribute_value"));
  return KTEST_SUCCESS;
}

static int test_override_exisiting_devprop_attr(void) {
  devprop_attr_t *attr;
  devprop_res_key_t key = FDT_PATH;
  set_device_prop_attr(gt_pci, key, "attribute_value");
  set_device_prop_attr(gt_pci, key, "overridden_value");
  attr = get_device_prop_attr(gt_pci, key);

  assert(!strcmp(attr->value.str_value, "overridden_value"));
  return KTEST_SUCCESS;
}

KTEST_ADD(operate_on_devprops_attrs, test_operate_on_devprops_attrs, 0);
KTEST_ADD(override_exisiting_devprop_attr, test_override_exisiting_devprop_attr,
          0);
