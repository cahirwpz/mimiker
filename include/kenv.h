#ifndef _SYS_KENV_H
#define _SYS_KENV_H

void setup_kenv(int pfm_argc, char **pfm_argv);

char *kenv_get(const char *key);

char **kenv_get_user_argv(void);
char **kenv_get_user_envv(void);

#endif /* !_SYS_KENV_H */
