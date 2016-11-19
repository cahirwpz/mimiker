#include <stdc.h>
#include <args.h>

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; ++i) {
    kprintf("arg %d: >%s<\n", i, argv[i]);
  }

  kprintf("value of param 'root': %s\n", args_get_value("root", argc, argv));
  kprintf("value of param 'foo': %s\n", args_get_value("foo", argc, argv));
  kprintf("is flag 'debug' set?: %d\n", args_get_flag("debug", argc, argv));

  return 0;
}
