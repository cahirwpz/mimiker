#include <stdc.h>
#include <ktest.h>
#include <klog.h>
#include <fdt.h>

#define PATHBUF_LEN 100

extern uint8_t __test_dtb_start[];
const void *fdt = (const void *)__test_dtb_start;

static int test_fdt_path_offset(void) {
  assert(fdt_path_offset(fdt, "/pci@0/isa@0") != -1);
  assert(fdt_path_offset(fdt, "/pci@0/isa@0/rtc@0") != -1);
  assert(fdt_path_offset(fdt, "/pci/isa") != -1);
  assert(fdt_path_offset(fdt, "/pci/isa/rtc@0") != -1);
  assert(fdt_path_offset(fdt, "/non/existing/path") == -1);
  return KTEST_SUCCESS;
}

static int test_fdt_get_path(void) {
  const char *expected_path = "/pci@0/isa@/rtc@0";
  char actual_path[PATHBUF_LEN];

  int offset = fdt_path_offset(fdt, expected_path);
  fdt_get_path(fdt, offset, actual_path, PATHBUF_LEN);

  assert(!strcmp(actual_path, expected_path));
  return KTEST_SUCCESS;
}

static int test_fdt_getprop(void) {
  int proplen;
  int nodeoffset;
  const fdt_property_t *prop_ptr;
  const char *node_path = "/pci@0/isa@0/uart@0";

  nodeoffset = fdt_path_offset(fdt, node_path);
  prop_ptr = fdt_getprop(fdt, nodeoffset, "status", &proplen);
  klog("prop: %s", (char*)prop_ptr->data);
  assert(!strcmp(prop_ptr->data, "ns16550"));

  nodeoffset = fdt_path_offset(fdt, node_path);
  prop_ptr = fdt_getprop(fdt, nodeoffset, "non_existing_prop", &proplen);
  assert(prop_ptr == NULL);

  return KTEST_SUCCESS;
}

KTEST_ADD(fdt_path_offset, test_fdt_path_offset, 0);
KTEST_ADD(fdt_get_path, test_fdt_get_path, 0);
KTEST_ADD(fdt_getprop, test_fdt_getprop, 0);
