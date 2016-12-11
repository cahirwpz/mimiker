#include <mips/stack.h>
#include <common.h>
#include <string.h>
#include <stdc.h>

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
void prepare_program_stack(const exec_args_t *args, vm_addr_t *stack_bottom_p) {

  vm_addr_t stack_end = *stack_bottom_p;
  /* Begin by calculting arguments total size. This has to be done */
  /* in advance, because stack grows downwards. */
  size_t total_arg_size = 0;
  for (size_t i = 0; i < args->argc; i++) {
    total_arg_size += roundup(strlen(args->argv[i]) + 1, 4);
  }
  /* Store arguments, creating the argument vector. */
  vm_addr_t arg_vector[args->argc];
  vm_addr_t p = *stack_bottom_p - total_arg_size;
  for (int i = 0; i < args->argc; i++) {
    size_t n = strlen(args->argv[i]) + 1;
    arg_vector[i] = p;
    memcpy((uint8_t *)p, args->argv[i], n);
    p += n;
    /* Align to word size */
    p = align(p, 4);
  }
  assert(p == stack_end);

  /* Move the stack down and align it so that the top of the stack will be
     8-byte alligned. */
  *stack_bottom_p = (*stack_bottom_p - total_arg_size) & 0xfffffff8;
  if (args->argc % 2 == 0)
    *stack_bottom_p -= 4;

  /* Now, place the argument vector on the stack. */
  size_t arg_vector_size = sizeof(arg_vector);
  vm_addr_t argv = *stack_bottom_p - arg_vector_size;
  memcpy((void *)argv, &arg_vector, arg_vector_size);
  /* Move the stack down. */
  *stack_bottom_p = *stack_bottom_p - arg_vector_size;

  /* Finally, place argc on the stack */
  *stack_bottom_p = *stack_bottom_p - sizeof(vm_addr_t);
  vm_addr_t *stack_args = (vm_addr_t *)*stack_bottom_p;
  *stack_args = args->argc;

  /* Top of the stack must be 8-byte alligned. */
  assert((*stack_bottom_p & 0x7) == 0);

  /* TODO: Environment */
}
