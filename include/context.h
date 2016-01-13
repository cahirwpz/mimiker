#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "common.h"
#include <stddef.h>

struct ctx_struct{
    /* Registers stored (in that particular order):
     *
     * ra
     * sp
     * gp
     * s0 s1 s2 s3 s4 s5 s6 s7 s8
     *
     * Since context-saving procedures work as function calls, several
     * registers do not require saving because they are not saved by
     * called functions anyway. This includes:
     * a0-a3, v0-v1, t0-t9, k0-k1
     *
     * The program counter does not require storing, because the
     * context restoring procedures work as function calls, so it is
     * sufficient to replace the return address register.
     */
    word_t registers[12];
};

/* Stores the current execution context to the given structure.
 * Returns 0 when returning directly, or 1 when returning as a result
 * of ctx_load.
 */
uint32_t ctx_store(struct ctx_struct* ctx) __attribute__((warn_unused_result));

/* Restores a previously stored context. This function does not
 * return. The control flow jumps to the corresponding ctx_store.
 */
void ctx_load(const struct ctx_struct* ctx) __attribute__((noreturn));

#endif // __CONTEXT_H__
