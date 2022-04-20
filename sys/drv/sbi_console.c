#include <sys/console.h>
#include <riscv/sbi.h>

static void sbi_console_init(console_t *cn __unused) {
}

static int sbi_console_getc(console_t *cn __unused) {
  return sbi_console_getchar();
}

static void sbi_console_putc(console_t *cn __unused, int c) {
  sbi_console_putchar(c);
}

static console_t sbi_console = {
  .cn_init = sbi_console_init,
  .cn_getc = sbi_console_getc,
  .cn_putc = sbi_console_putc,
  .cn_prio = 1,
};

CONSOLE_ADD(sbi_console);
