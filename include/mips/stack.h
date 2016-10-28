#ifndef __MIPS_STACK_H__
#define __MIPS_STACK_H__
#include <exec.h>
#include <vm.h>

void prepare_program_stack(const exec_args_t *args, vm_addr_t *stack_bottom_p);

#endif // __MIPS_STACK_H__
