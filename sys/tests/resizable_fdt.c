#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ktest.h>

static int test_resizable_fdt(void) {
  fdtab_t *fdt_test = fdtab_create();

  for (int i = 0; i < 100; i++) {
    file_t *tmp_file = file_alloc();
    int new_fd;
    fdtab_install_file(fdt_test, tmp_file, 0, &new_fd);
  }

  fdtab_t *another_fdt_test = fdtab_copy(fdt_test);

  fdtab_drop(fdt_test);
  fdtab_drop(another_fdt_test);

  return KTEST_SUCCESS;
}

KTEST_ADD(resizable_fdt, test_resizable_fdt, 0);
