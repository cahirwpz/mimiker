#ifndef _SYS_CONSOLE_H_
#define _SYS_CONSOLE_H_

#include <sys/linker_set.h>

typedef struct console console_t;

typedef void (*cn_init_t)(console_t *);
typedef int (*cn_getc_t)(console_t *);
typedef void (*cn_putc_t)(console_t *, int);

struct console {
  cn_init_t cn_init;
  cn_getc_t cn_getc;
  cn_putc_t cn_putc;
  int cn_prio;
};

#define CONSOLE_ADD(name) SET_ENTRY(cn_table, name)

/*! \brief Called during kernel initialization. */
void init_cons(void);

int cn_getc(void);
void cn_putc(int c);
int cn_puts(const char *s);
int cn_write(const char *s, unsigned n);

#endif /* !_SYS_CONSOLE_H */
