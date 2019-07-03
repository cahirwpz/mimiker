#ifndef _SYS_STACK_H_
#define _SYS_STACK_H_

#include <sys/cdefs.h>

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

#define stack_alloc_s(sp, type) stack_alloc(sp, sizeof(type))

#endif /* !_SYS_STACK_H_ */
