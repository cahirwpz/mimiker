#ifndef __ARGS_H__
#define __ARGS_H__

void kernel_args_parse(int argc, char **argv, char **envp);

/* Searches for key=val argument in kernel args, and returns a pointer to the
 * value string, or NULL if not found. On multiple matches, all but first are
 * ignored. */
const char *kernel_args_get(const char *key);

/* Searches for flag in kernel arguments, and returns 1 if found, 0 otherwise.
 */
int kernel_args_get_flag(const char *flag);

#endif // __ARGS_H__
