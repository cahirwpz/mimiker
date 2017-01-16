#ifndef __EXEC_H__
#define __EXEC_H__

#include <stdint.h>
#include <stddef.h>

typedef struct exec_args {
  /* Program name. Temporarily this is just a hardcoded text
   * identifier of an embedded ELF image to use, eventually this
   * would become a path to the executable (or an open file
   * descriptor). */
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
