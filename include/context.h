#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <common.h>
#include <mips/mips.h>
#include <mips/ctx.h>

typedef struct stack {
  void  *stk_base; /* stack base */
  size_t stk_size; /* stack length */
} stack_t;

/*
 * Since context-saving procedures work as function calls, several registers do
 * not require saving because they are not saved by called functions anyway.
 * This includes: $at, $a0-$a3, $v0-$v1, $t0-$t9, $k0-$k1.
 *
 * The program counter does not require storing, because the context restoring
 * procedures work as function calls, so it is sufficient to replace the return
 * address register.
 */

typedef struct ctx {
  reg_t reg[REG_NUM];
} ctx_t;

/*
 * Loads a fresh context. This function does not return.
 * The control flow jumps to place pointed by REG_PC.
 */
noreturn void ctx_boot(const ctx_t *ctx);

/*
 * This function sets the contents of a context struct, zeroing it's
 * all registers except for return address, which is set to @target,
 * stack pointer, which is set to @stack, and global pointer, which is
 * set to @gp.
 *
 * WARNING: The target procedure MUST NOT RETURN. The result of such
 * event is undefined, but will generally restart the target function.
 */
void ctx_init(ctx_t *ctx, void (*target)(), void *sp);

/* This function stores the current context to @from, and resumes the
 * context stored in @to. It does not return immediatelly, it returns
 * only when the @from context is resumed. */
void ctx_switch(ctx_t *from, ctx_t *to);

#endif // __CONTEXT_H__
