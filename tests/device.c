#include <stdc.h>
#include <ktest.h>
#include <klog.h>
#include <device.h>

extern device_t *gt_pci;

static int test_device_get_fullname(void) {
    // TODO: This test depends on the fact that gt_pci exists
    char *expected_name = "pci@0";
    char name_buff[MAX_DEV_NAME_LEN];
    device_get_fullname(gt_pci, name_buff, MAX_DEV_NAME_LEN);

    assert(strcmp(name_buff, expected_name) == 0);

    return KTEST_SUCCESS;
}

static int test_device_construct_fullpath(void) {
    // TODO: This test depends on the fact that gt_pci exists
    // TODO: Test more complex paths
    char *expected_path = "/pci@0";
    char path_buff[PATHBUF_LEN];
    device_construct_fullpath(gt_pci, path_buff, PATHBUF_LEN);
    assert(strcmp(path_buff, expected_path) == 0);
    return KTEST_SUCCESS;
}

KTEST_ADD(device_get_fullname, test_device_get_fullname, 0);
KTEST_ADD(device_construct_fullpath, test_device_construct_fullpath, 0);
