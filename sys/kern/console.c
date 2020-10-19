#include <sys/mimiker.h>
#include <sys/console.h>
#include <sys/linker_set.h>

static void dummy_init(console_t *dev __unused) {
}
static void dummy_putc(console_t *dev __unused, int c __unused) {
}
static int dummy_getc(console_t *dev __unused) {
  return 0;
}

static console_t dummy_console = {
  .cn_init = dummy_init,
  .cn_getc = dummy_getc,
  .cn_putc = dummy_putc,
  .cn_prio = -10,
};

CONSOLE_ADD(dummy_console);

static console_t *cn = &dummy_console;

void init_cons(void) {
  SET_DECLARE(cn_table, console_t);
  int prio = INT_MIN;
  console_t **ptr;
  SET_FOREACH (ptr, cn_table) {
    console_t *cn_ = *ptr;
    cn_->cn_init(cn_);
    if (prio < cn_->cn_prio) {
      prio = cn_->cn_prio;
      cn = cn_;
    }
  }
}

void cn_putc(int c) {
  cn->cn_putc(cn, c);
}

int cn_getc(void) {
  return cn->cn_getc(cn);
}

int cn_puts(const char *str) {
  int n = 0;
  while (*str) {
    cn_putc(*str++);
    n++;
  }
  cn_putc('\n');
  return n + 1;
}

int cn_write(const char *str, unsigned n) {
  while (n--)
    cn_putc(*str++);
  return n;
}
