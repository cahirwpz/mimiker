#ifndef _SYS_KSTACK_H_
#define _SYS_KSTACK_H_

#include <sys/cdefs.h>

/*! \brief Thread stack structure. */
typedef struct kstack {
  void *stk_base;  /*!< stack base */
  size_t stk_size; /*!< stack length */
} kstack_t;

/*! \brief Calculate pointer to bottom of the stack.
 *
 * Stack grows down, so bottom has the highest address.
 */
static inline void *stack_bottom(kstack_t *stk) {
  return stk->stk_base + stk->stk_size;
}

/*! \brief Allocate n bytes on stack. */
static inline void *kstack_alloc(void *sp, unsigned n) {
  return sp - n;
}

#define kstack_alloc_s(sp, type) kstack_alloc(sp, sizeof(type))

#endif /* !_SYS_KSTACK_H_ */
