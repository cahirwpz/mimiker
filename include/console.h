#ifndef _SYS_CONSOLE_H_
#define _SYS_CONSOLE_H_

#include <linker_set.h>

typedef struct console console_t;

typedef void (*cn_init_t)(console_t *);
typedef int (*cn_getc_t)(console_t *);
typedef void (*cn_putc_t)(console_t *, int);

typedef struct console {
  cn_init_t cn_init;
  cn_getc_t cn_getc;
  cn_putc_t cn_putc;
  int cn_prio;
} console_t;

#define CONSOLE_ADD(name) SET_ENTRY(cn_table, name)

void cn_init();
int cn_getc();
void cn_putc(int c);
int cn_puts(const char *s);
int cn_write(const char *s, unsigned n);

#endif
