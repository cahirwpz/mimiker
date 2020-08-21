#include <sys/kenv.h>
#include <sys/libkern.h>

static char **_kenvp;
static char **_kinit = (char * [2]){NULL, NULL};

void init_kenv(char **kenvp) {
  _kenvp = kenvp;

  /* Let's find "--". After we set it to NULL it's going to become first element
   * of init arguments. */
  for (char **argp = kenvp; *argp; argp++) {
    if (strcmp("--", *argp) == 0) {
      *argp++ = NULL;
      _kinit = argp;
    }
  }

  _kinit[0] = kenv_get("init");
}

char *kenv_get(const char *key) {
  unsigned n = strlen(key);

  for (char **argp = _kenvp; *argp; argp++) {
    char *arg = *argp;
    if ((strncmp(arg, key, n) == 0) && (arg[n] == '='))
      return arg + n + 1;
  }

  return NULL;
}

u_long kenv_get_ulong(const char *key) {
  const char *s = kenv_get(key);

  if (s == NULL)
    return 0;

  int base = 10;

  /* skip '0x' and note it's hexadecimal */
  if (s[0] == '0' && s[1] == 'x') {
    s += 2;
    base = 16;

    /* truncate long long to long */
    int n = strlen(s);
    if (n > 8)
      s += n - 8;
  }

  return strtoul(s, NULL, base);
}

char **kenv_get_init(void) {
  return _kinit;
}
