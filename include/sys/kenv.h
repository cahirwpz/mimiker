#ifndef _SYS_KENV_H_
#define _SYS_KENV_H_

#include <sys/types.h>

/* Initialize kenv with parameters and arguments for init program. */
void init_kenv(char **kenvp);

/* Fetch string value with given key. */
char *kenv_get(const char *key);

/* Fetch integer value with given key. */
u_long kenv_get_ulong(const char *key);

/* Fetch argv vector to be passed to init program. */
char **kenv_get_init(void);

#endif /* !_SYS_KENV_H_ */
