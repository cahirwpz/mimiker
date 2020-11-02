#ifndef _SYS_CMDLINE_H_
#define _SYS_CMDLINE_H_

#include <sys/kstack.h>

size_t cmdline_count_tokens(const char *str);
char **cmdline_extract_tokens(kstack_t *stk, const char *str, char **tokens_p);

#endif /* !_SYS_CMDLINE_H_ */
