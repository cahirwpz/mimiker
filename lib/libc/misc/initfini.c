#include <sys/cdefs.h>
#include <stdbool.h>

void _libc_init(void) __attribute__((__constructor__, __used__));

void __libc_atexit_init(void);
void __libc_env_init(void);

static bool libc_initialised;

void __section(".text.startup") _libc_init(void) {
  if (libc_initialised)
    return;

  libc_initialised = 1;

  /* Initialize the atexit mutexes */
  __libc_atexit_init();

  /* Initialize environment memory RB tree. */
  __libc_env_init();
}
