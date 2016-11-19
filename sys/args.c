#include <args.h>
#include <string.h>

int args_get_flag(const char *name, int argc, char *argv[]) {
  for (int i = 0; i < argc; ++i)
    if (strcmp(name, argv[i]) == 0)
      return 1;
  return 0;
}

const char *args_get_value(const char *name, int argc, char *argv[]) {
  int n = strlen(name);
  for (int i = 0; i < argc; ++i)
    if (strncmp(name, argv[i], n) == 0 && argv[i][n] == '=')
      return argv[i] + n + 1;
  return NULL;
}
