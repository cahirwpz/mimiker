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
ASSYM(TDF_NEEDLOCK, TDF_NEEDLOCK);
ASSYM(TDF_USESFPU, TDF_USESFPU);

ASSYM(TD_PROC, offsetof(thread_t, td_proc));
ASSYM(TD_UCTX, offsetof(thread_t, td_uctx));
ASSYM(TD_UCTX_FPU, offsetof(thread_t, td_uctx_fpu));
ASSYM(TD_KFRAME, offsetof(thread_t, td_kframe));
ASSYM(TD_KCTX, offsetof(thread_t, td_kctx));
ASSYM(TD_KSTACK, offsetof(thread_t, td_kstack));
ASSYM(TD_FLAGS, offsetof(thread_t, td_flags));
ASSYM(TD_ONFAULT, offsetof(thread_t, td_onfault));
ASSYM(TD_IDNEST, offsetof(thread_t, td_idnest));
ASSYM(TD_SPIN, offsetof(thread_t, td_spin));

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

ASSYM(FPU_CTX_F0, offsetof(fpu_ctx_t, f0));
ASSYM(FPU_CTX_F1, offsetof(fpu_ctx_t, f1));
ASSYM(FPU_CTX_F2, offsetof(fpu_ctx_t, f2));
ASSYM(FPU_CTX_F3, offsetof(fpu_ctx_t, f3));
ASSYM(FPU_CTX_F4, offsetof(fpu_ctx_t, f4));
ASSYM(FPU_CTX_F5, offsetof(fpu_ctx_t, f5));
ASSYM(FPU_CTX_F6, offsetof(fpu_ctx_t, f6));
ASSYM(FPU_CTX_F7, offsetof(fpu_ctx_t, f7));
ASSYM(FPU_CTX_F8, offsetof(fpu_ctx_t, f8));
ASSYM(FPU_CTX_F9, offsetof(fpu_ctx_t, f9));
ASSYM(FPU_CTX_F10, offsetof(fpu_ctx_t, f10));
ASSYM(FPU_CTX_F11, offsetof(fpu_ctx_t, f11));
ASSYM(FPU_CTX_F12, offsetof(fpu_ctx_t, f12));
ASSYM(FPU_CTX_F13, offsetof(fpu_ctx_t, f13));
ASSYM(FPU_CTX_F14, offsetof(fpu_ctx_t, f14));
ASSYM(FPU_CTX_F15, offsetof(fpu_ctx_t, f15));
ASSYM(FPU_CTX_F16, offsetof(fpu_ctx_t, f16));
ASSYM(FPU_CTX_F17, offsetof(fpu_ctx_t, f17));
ASSYM(FPU_CTX_F18, offsetof(fpu_ctx_t, f18));
ASSYM(FPU_CTX_F19, offsetof(fpu_ctx_t, f19));
ASSYM(FPU_CTX_F20, offsetof(fpu_ctx_t, f20));
ASSYM(FPU_CTX_F21, offsetof(fpu_ctx_t, f21));
ASSYM(FPU_CTX_F22, offsetof(fpu_ctx_t, f22));
ASSYM(FPU_CTX_F23, offsetof(fpu_ctx_t, f23));
ASSYM(FPU_CTX_F24, offsetof(fpu_ctx_t, f24));
ASSYM(FPU_CTX_F25, offsetof(fpu_ctx_t, f25));
ASSYM(FPU_CTX_F26, offsetof(fpu_ctx_t, f26));
ASSYM(FPU_CTX_F27, offsetof(fpu_ctx_t, f27));
ASSYM(FPU_CTX_F28, offsetof(fpu_ctx_t, f28));
ASSYM(FPU_CTX_F29, offsetof(fpu_ctx_t, f29));
ASSYM(FPU_CTX_F30, offsetof(fpu_ctx_t, f30));
ASSYM(FPU_CTX_F31, offsetof(fpu_ctx_t, f31));
ASSYM(FPU_CTX_FSR, offsetof(fpu_ctx_t, fsr));

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
