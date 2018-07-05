#ifndef _SYS_STACK_H_
#define _SYS_STACK_H_

#include <common.h>

typedef struct exec_args exec_args_t;

/*! \brief Thread stack structure. */
typedef struct stack {
  void *stk_base;  /*!< stack base */
  size_t stk_size; /*!< stack length */
} stack_t;

/*! \brief Calculate pointer to bottom of the stack.
 *
 * Stack grows down, so bottom has the highest address.
 */
static inline void *stack_bottom(stack_t *stk) {
  return stk->stk_base + stk->stk_size;
}

/*! \brief Allocate n bytes on stack. */
static inline void *stack_alloc(void *sp, unsigned n) {
  return sp - n;
}

/*! \brief Initialize user stack for entry from kernel space. */
void stack_user_entry_setup(const exec_args_t *args, vaddr_t *sp_p);

#define stack_alloc_s(sp, type) stack_alloc(sp, sizeof(type))

#endif /* !_SYS_STACK_H_ */
