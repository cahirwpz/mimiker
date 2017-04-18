#ifndef __EXEC_H__
#define __EXEC_H__

#include <stdint.h>
#include <stddef.h>

typedef struct exec_args {
  /* Path to the executable. */
  const char *prog_name;
  /* Program arguments. These will get copied to the stack of the
   * starting process. */
  uint8_t argc;
  const char **argv;
  /* TODO: Environment */
} exec_args_t;

int get_elf_image(const exec_args_t *args, uint8_t **out_image,
                  size_t *out_size);

int do_exec(const exec_args_t *args);

#endif
