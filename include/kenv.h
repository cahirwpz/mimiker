#ifndef _SYS_KENV_H
#define _SYS_KENV_H

#include <stdbool.h>

void setup_kenv(int pfm_argc, char **pfm_argv, char **pfm_envv);
void print_kenv(void);

char *kenv_get(const char *key);

/* Accepts integer with base 10, or 16 if it starts with '0x' */
bool kenv_get_int(const char *key, int *val_p);

/* Returns number of slots filled in strv, without closing NULL, zero if key was
 * not found. */
int kenv_get_strv(const char *key, char **strv, size_t len);

#endif /* !_SYS_KENV_H */
