#include <mount.h>
#include <stdc.h>

int main() {
  vnode_t *dev_null;
  int error = vfs_lookup("/dev/null", &dev_null);
  kprintf("Error: %d\n", error);
  return 0;
}
