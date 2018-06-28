#include <common.h>
#include <stdc.h>
#include <stack.h>
#include <exec.h>
#include <systm.h>
#include <errno.h>

/* Places program args onto the stack.
 * Also modifies value pointed by stack_bottom_p to reflect on changed
 * stack bottom address.
 * The stack layout will be as follows:
 *
 *  ----------- stack segment high address
 *  | argv[n] |
 *  |   ...   |  each of argv[i] is a null-terminated string
 *  | argv[1] |
 *  | argv[0] |
 *  |---------|
 *  | argv    |  the argument vector storing pointers to argv[0..n]
 *  |---------|
 *  | argc    |  a single uint32 declaring the number of arguments (n)
 *  |---------|
 *  | program |
 *  |  stack  |
 *  |   ||    |
 *  |   \/    |
 *  |         |
 *  |   ...   |
 *  ----------- stack segment low address
 *
 *
 * After this function runs, the value pointed by stack_bottom_p will
 * be the address where argc is stored, which is also the bottom of
 * the now empty program stack, so that it can naturally grow
 * downwards.
 */

void patch_blob(void *blob, size_t addr, bool add) {

  size_t argc = ((size_t *)blob)[0];
  char **argv = (char **)(blob + sizeof(argc));

  size_t oldbase = (size_t)(blob);

  for (size_t i = 0; i < argc; i++) {

    argv[i] = argv[i] + (addr - oldbase);
  }
}

void stack_user_entry_setup(const exec_args_t *args,
                            vm_addr_t *stack_bottom_p) {

  size_t total_arg_size = args->bytes_written;

  assert((total_arg_size % 8) == 0);
  assert((*stack_bottom_p % 8) == 0);
  *stack_bottom_p = (*stack_bottom_p - total_arg_size);
  patch_blob(args->arg_blob, (size_t)(*stack_bottom_p), true);
  memcpy((uint8_t *)*stack_bottom_p, args->arg_blob, total_arg_size);

  /* /\* TODO: Environment *\/ */
}

typedef struct copy_ops {
  int (*cpybytes)(const void *saddr, void *daddr, size_t len);
  int (*cpystr)(const void *saddr, void *daddr, size_t len, size_t *lencopied);
} copy_ops_t;

int copy_ptrs(const char **user_argv, int8_t *blob, size_t blob_size,
              size_t *argc_out, copy_ops_t co) {

  const char **kern_argv = (const char **)blob;

  char *arg_ptr;
  const size_t ptr_size = sizeof(arg_ptr);

  for (int argc = 0; argc * ptr_size < blob_size; argc++) {
    int result = co.cpybytes(user_argv + argc, &arg_ptr, ptr_size);
    if (result < 0)
      return result;

    kern_argv[argc] = arg_ptr;
    if (arg_ptr != NULL) // Do we need to copy more arg ptrs?
      continue;

    // We copied all arg ptrs :)
    if (argc == 0) // Is argument list empty?
      return -EFAULT;

    // Its not empty and we copied all arg ptrs - Success!
    *argc_out = argc;
    return 0;
  }

  // We execeeded ARG_MAX while copying arguments
  return -E2BIG;
}

int copy_strings(size_t argc, const char **kern_argv, int8_t *dst,
                 size_t dst_size, size_t *bytes_written, copy_ops_t co) {

  // int8_t *args = argv_blob + *bytes_written;
  size_t argsize, i = 0;
  int result;

  do {

    result = co.cpystr(kern_argv[i], dst, dst_size - *bytes_written, &argsize);
    if (result < 0)
      return (result == -ENAMETOOLONG) ? -E2BIG : result;

    kern_argv[i] = (char *)(dst);
    argsize = roundup(argsize, 4);
    dst += argsize;
    *bytes_written += argsize;
    i++;

  } while ((i < argc) && (*bytes_written < dst_size));

  if (i < argc)
    return -E2BIG;

  *bytes_written = roundup(*bytes_written, 8);

  return 0;
}

int marshal_args(const char **argv, int8_t *dst, size_t dst_size,
                 size_t *bytes_written, copy_ops_t co) {

  assert(sizeof(vm_addr_t) == 4);
  assert(sizeof(size_t) == 4);
  assert(sizeof(char *) == 4);
  assert(dst_size >= 4);

  size_t argc;
  const char **kern_argv = (const char **)(dst + sizeof(argc));

  int result =
    copy_ptrs(argv, (int8_t *)kern_argv, dst_size - sizeof(argc), &argc, co);

  if (result < 0)
    return result;

  assert(argc > 0);

  *bytes_written = argc * sizeof(char *);

  ((size_t *)dst)[0] = argc;
  *bytes_written += sizeof(argc);
  *bytes_written = roundup(*bytes_written, 8);

  int8_t *args = dst + *bytes_written;

  return copy_strings(argc, kern_argv, args, dst_size, bytes_written, co);
}

copy_ops_t from_uspace = {.cpybytes = copyin, .cpystr = copyinstr};

int uspace_marshal_args(const char **argv, int8_t *dst, size_t dst_size,
                        size_t *bytes_written) {

  return marshal_args(argv, dst, dst_size, bytes_written, from_uspace);
}

int memcpy_wrapper(const void *src, void *dst, size_t dst_size) {

  memcpy(dst, src, dst_size);
  return 0;
}

int strlcpy_wrapper(const void *src, void *dst, size_t dst_size,
                    size_t *lencopied) {

  size_t act_size = strlen(src) + 1;
  int result = 0;

  strlcpy(dst, src, dst_size);

  if (act_size > dst_size) {
    result = -ENAMETOOLONG;
    act_size = dst_size;
  }

  *lencopied = act_size;
  return result;
}

copy_ops_t from_kspace = {.cpybytes = memcpy_wrapper,
                          .cpystr = strlcpy_wrapper};

int kspace_marshal_args(const char **argv, int8_t *dst, size_t dst_size,
                        size_t *bytes_written) {

  return marshal_args(argv, dst, dst_size, bytes_written, from_kspace);
}
