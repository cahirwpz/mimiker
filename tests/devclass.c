#include <ktest.h>
#include <devclass.h>

static int test_devclass_find(void) {
  devclass_t *dc;

  dc = devclass_find("pci");
  assert(strcmp(dc->name, "pci") == 0);

  dc = devclass_find("this_devclass_surely_doesnt_exist");
  assert(dc == NULL);

  return KTEST_SUCCESS;
}

KTEST_ADD(devclass_find, test_devclass_find, 0);
