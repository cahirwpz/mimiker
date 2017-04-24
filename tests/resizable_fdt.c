#include <file.h>
#include <filedesc.h>
#include <ktest.h>

static int test_resizable_fdt() {
  fdtab_t *fdt_test = fdtab_alloc();
  file_init();

  for (int i = 0; i < 100; i++) {
    file_t *tmp_file = file_alloc();
    int new_fd;
    fdtab_install_file(fdt_test, tmp_file, &new_fd);
  }

  fdtab_destroy(fdt_test);

  return KTEST_SUCCESS;
}

KTEST_ADD(resizable_fdt, test_resizable_fdt, 0);
