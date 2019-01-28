

void setup_kenv(int pfm_argc, char **pfm_argv);

char *kenv_get(const char *key);

int kenv_get_argc(void);
char **kenv_get_argv(void);
char **kenv_get_user_argv(void);
char **kenv_get_user_envv(void);
