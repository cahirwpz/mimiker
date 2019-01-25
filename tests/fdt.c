#include <stdc.h>
#include <ktest.h>
#include <klog.h>
#include <fdt.h>
#include <devhints.h>
#include <devclass.h>

extern uint8_t __malta_dtb_start[];
const void *fdt = (const void *)__malta_dtb_start;

static int test_fdt_path_offset(void) {
  // TODO: This test depends on actual dts file, which may change in future
  // and is platform-depentend
  assert(fdt_path_offset(fdt, "/pci@0/isa@1000") != -1);
  assert(fdt_path_offset(fdt, "/pci/isa") != -1);
  assert(fdt_path_offset(fdt, "/non/existing/path") == -1);
  return KTEST_SUCCESS;
}

static int test_fdt_get_path(void) {
  // TODO: This test depends on actual dts file, which may change in future
  // and is platform-depentend
  const char *expected_path = "/pci@0/isa@1000";
  char actual_path[PATHBUF_LEN];

  int offset = fdt_path_offset(fdt, expected_path);
  fdt_get_path(fdt, offset, actual_path, PATHBUF_LEN);

  assert(strcmp(actual_path, expected_path) == 0);
  return KTEST_SUCCESS;
}

static int test_fdt_getprop(void) {
  int proplen;
  int nodeoffset;
  const fdt_property_t *prop_ptr;
  const char *node_path = "/pci@0/isa@1000/atkbdc@60";

  nodeoffset = fdt_path_offset(fdt, node_path);
  prop_ptr = fdt_getprop(fdt, nodeoffset, "compatible", &proplen);
  assert(strcmp(prop_ptr->data, "atkbdc"));

  nodeoffset = fdt_path_offset(fdt, node_path);
  prop_ptr = fdt_getprop(fdt, nodeoffset, "non_existing_prop", &proplen);
  assert(prop_ptr == NULL);

  return KTEST_SUCCESS;
}

/*
static int test_for_each_subnode(void) {
    // TODO: do we want this kind of test?
    // FDT_FOR_EACH_SUBNODE(child_nodeoffset, fdt, bus_nodeoffset) {
    // }
}
*/

KTEST_ADD(fdt_path_offset, test_fdt_path_offset, 0);
KTEST_ADD(fdt_get_path, test_fdt_get_path, 0);
KTEST_ADD(fdt_getprop, test_fdt_getprop, 0);
