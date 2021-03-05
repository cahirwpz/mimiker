#include <sys/klog.h>
#include <sys/ktest.h>
#include <sys/fdt.h>

#define BUF_SIZE 64

extern uint8_t __fake_dtb_start[];
static const void *fdt = (const void *)__fake_dtb_start;

static int test_fdt_get_path(void) {
  const char *path = "/root@0/cbus@0/serial@0";
  char buf[BUF_SIZE];

  int offset = fdt_path_offset(fdt, path);
  fdt_get_path(fdt, offset, buf, BUF_SIZE);

  assert(!strcmp(path, buf));

  return KTEST_SUCCESS;
}

static int test_fdt_get_name(void) {
  const char *path = "/root@0/pci@0/isa@0/serial@1";
  const char *name = "serial@1";
  int len;

  int offset = fdt_path_offset(fdt, path);
  const char *retval = fdt_get_name(fdt, offset, &len);

  assert(retval);
  assert(len == (int)strlen(name));
  assert(!(strcmp(name, retval)));

  return KTEST_SUCCESS;
}

static int test_fdt_subnode(void) {
  const char *path = "/root@0/pci@0/isa@0";
  const char *name_first = "rtc@0";
  const char *name_next = "atkbdc@0";
  const char *retval;
  int offset;
  int len;

  offset = fdt_path_offset(fdt, path);
  offset = fdt_first_subnode(fdt, offset);
  assert(offset != -FDT_ERR_NOTFOUND);
  retval = fdt_get_name(fdt, offset, &len);

  assert(retval);
  assert(len == (int)strlen(name_first));
  assert(!(strcmp(name_first, retval)));

  offset = fdt_next_subnode(fdt, offset);
  assert(offset != -FDT_ERR_NOTFOUND);
  retval = fdt_get_name(fdt, offset, &len);

  assert(retval);
  assert(len == (int)strlen(name_next));
  assert(!(strcmp(name_next, retval)));

  return KTEST_SUCCESS;
}

static int test_fdt_getprop(void) {
  const char *path = "/root@0/pci@0/isa@0";
  const char *name = "compatible";
  const char *prop;
  int offset;
  int len;

  offset = fdt_path_offset(fdt, path);
  prop = (const char *)fdt_getprop(fdt, offset, name, &len);

  assert(prop);
  /* NULL byte */
  assert(len == (int)strlen("isa") + 1);
  assert(!strcmp(prop, "isa"));

  return KTEST_SUCCESS;
}

KTEST_ADD(fdt_get_path, test_fdt_get_path, 0);
KTEST_ADD(fdt_get_name, test_fdt_get_name, 0);
KTEST_ADD(fst_subnode, test_fdt_subnode, 0);
KTEST_ADD(fst_getprop, test_fdt_getprop, 0);
