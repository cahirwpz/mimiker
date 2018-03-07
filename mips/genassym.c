#include <assym.h>
#include <context.h>
#include <thread.h>
#include <pcpu.h>
#include <proc.h>
#include <mips/asm.h>
#include <mips/ctx.h>
#include <mips/exc.h>

ASSYM(TDF_NEEDSWITCH, TDF_NEEDSWITCH);
ASSYM(TDF_NEEDSIGCHK, TDF_NEEDSIGCHK);

ASSYM(TD_PROC, offsetof(thread_t, td_proc));
ASSYM(TD_UCTX, offsetof(thread_t, td_uctx));
ASSYM(TD_UCTX_FPU, offsetof(thread_t, td_uctx_fpu));
ASSYM(TD_KFRAME, offsetof(thread_t, td_kframe));
ASSYM(TD_KCTX, offsetof(thread_t, td_kctx));
ASSYM(TD_KSTACK, offsetof(thread_t, td_kstack));
ASSYM(TD_FLAGS, offsetof(thread_t, td_flags));
ASSYM(TD_ONFAULT, offsetof(thread_t, td_onfault));
ASSYM(TD_IDNEST, offsetof(thread_t, td_idnest));

ASSYM(STK_BASE, offsetof(stack_t, stk_base));
ASSYM(STK_SIZE, offsetof(stack_t, stk_size));

ASSYM(CTX_S0, offsetof(ctx_t, s0));
ASSYM(CTX_S1, offsetof(ctx_t, s1));
ASSYM(CTX_S2, offsetof(ctx_t, s2));
ASSYM(CTX_S3, offsetof(ctx_t, s3));
ASSYM(CTX_S4, offsetof(ctx_t, s4));
ASSYM(CTX_S5, offsetof(ctx_t, s5));
ASSYM(CTX_S6, offsetof(ctx_t, s6));
ASSYM(CTX_S7, offsetof(ctx_t, s7));
ASSYM(CTX_GP, offsetof(ctx_t, gp));
ASSYM(CTX_SP, offsetof(ctx_t, sp));
ASSYM(CTX_FP, offsetof(ctx_t, fp));
ASSYM(CTX_SR, offsetof(ctx_t, sr));
ASSYM(CTX_PC, offsetof(ctx_t, pc));

ASSYM(CTX_S0, offsetof(ctx_t, s0));
ASSYM(CTX_S1, offsetof(ctx_t, s1));
ASSYM(CTX_S2, offsetof(ctx_t, s2));
ASSYM(CTX_S3, offsetof(ctx_t, s3));
ASSYM(CTX_S4, offsetof(ctx_t, s4));
ASSYM(CTX_S5, offsetof(ctx_t, s5));
ASSYM(CTX_S6, offsetof(ctx_t, s6));
ASSYM(CTX_S7, offsetof(ctx_t, s7));
ASSYM(CTX_GP, offsetof(ctx_t, gp));
ASSYM(CTX_SP, offsetof(ctx_t, sp));
ASSYM(CTX_FP, offsetof(ctx_t, fp));
ASSYM(CTX_SR, offsetof(ctx_t, sr));
ASSYM(CTX_PC, offsetof(ctx_t, pc));

ASSYM(EXC_AT, offsetof(exc_frame_t, at));
ASSYM(EXC_V0, offsetof(exc_frame_t, v0));
ASSYM(EXC_V1, offsetof(exc_frame_t, v1));
ASSYM(EXC_A0, offsetof(exc_frame_t, a0));
ASSYM(EXC_A1, offsetof(exc_frame_t, a1));
ASSYM(EXC_A2, offsetof(exc_frame_t, a2));
ASSYM(EXC_A3, offsetof(exc_frame_t, a3));
ASSYM(EXC_T0, offsetof(exc_frame_t, t0));
ASSYM(EXC_T1, offsetof(exc_frame_t, t1));
ASSYM(EXC_T2, offsetof(exc_frame_t, t2));
ASSYM(EXC_T3, offsetof(exc_frame_t, t3));
ASSYM(EXC_T4, offsetof(exc_frame_t, t4));
ASSYM(EXC_T5, offsetof(exc_frame_t, t5));
ASSYM(EXC_T6, offsetof(exc_frame_t, t6));
ASSYM(EXC_T7, offsetof(exc_frame_t, t7));
ASSYM(EXC_S0, offsetof(exc_frame_t, s0));
ASSYM(EXC_S1, offsetof(exc_frame_t, s1));
ASSYM(EXC_S2, offsetof(exc_frame_t, s2));
ASSYM(EXC_S3, offsetof(exc_frame_t, s3));
ASSYM(EXC_S4, offsetof(exc_frame_t, s4));
ASSYM(EXC_S5, offsetof(exc_frame_t, s5));
ASSYM(EXC_S6, offsetof(exc_frame_t, s6));
ASSYM(EXC_S7, offsetof(exc_frame_t, s7));
ASSYM(EXC_T8, offsetof(exc_frame_t, t8));
ASSYM(EXC_T9, offsetof(exc_frame_t, t9));
ASSYM(EXC_GP, offsetof(exc_frame_t, gp));
ASSYM(EXC_SP, offsetof(exc_frame_t, sp));
ASSYM(EXC_FP, offsetof(exc_frame_t, fp));
ASSYM(EXC_RA, offsetof(exc_frame_t, ra));
ASSYM(EXC_LO, offsetof(exc_frame_t, lo));
ASSYM(EXC_HI, offsetof(exc_frame_t, hi));
ASSYM(EXC_PC, offsetof(exc_frame_t, pc));
ASSYM(EXC_SR, offsetof(exc_frame_t, sr));
ASSYM(EXC_BADVADDR, offsetof(exc_frame_t, badvaddr));
ASSYM(EXC_CAUSE, offsetof(exc_frame_t, cause));

ASSYM(EXC_FRAME_SIZ, sizeof(exc_frame_t) + CALLFRAME_SIZ);

ASSYM(P_USPACE, offsetof(proc_t, p_uspace));

ASSYM(PCPU_CURTHREAD, offsetof(pcpu_t, curthread));
