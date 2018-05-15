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

ASSYM(TD_PROC, offsetof(thread_t, td_proc));
ASSYM(TD_UFRAME, offsetof(thread_t, td_uframe));
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

ASSYM(EXC_FPU_F0, offsetof(exc_frame_t, f0));
ASSYM(EXC_FPU_F1, offsetof(exc_frame_t, f1));
ASSYM(EXC_FPU_F2, offsetof(exc_frame_t, f2));
ASSYM(EXC_FPU_F3, offsetof(exc_frame_t, f3));
ASSYM(EXC_FPU_F4, offsetof(exc_frame_t, f4));
ASSYM(EXC_FPU_F5, offsetof(exc_frame_t, f5));
ASSYM(EXC_FPU_F6, offsetof(exc_frame_t, f6));
ASSYM(EXC_FPU_F7, offsetof(exc_frame_t, f7));
ASSYM(EXC_FPU_F8, offsetof(exc_frame_t, f8));
ASSYM(EXC_FPU_F9, offsetof(exc_frame_t, f9));
ASSYM(EXC_FPU_F10, offsetof(exc_frame_t, f10));
ASSYM(EXC_FPU_F11, offsetof(exc_frame_t, f11));
ASSYM(EXC_FPU_F12, offsetof(exc_frame_t, f12));
ASSYM(EXC_FPU_F13, offsetof(exc_frame_t, f13));
ASSYM(EXC_FPU_F14, offsetof(exc_frame_t, f14));
ASSYM(EXC_FPU_F15, offsetof(exc_frame_t, f15));
ASSYM(EXC_FPU_F16, offsetof(exc_frame_t, f16));
ASSYM(EXC_FPU_F17, offsetof(exc_frame_t, f17));
ASSYM(EXC_FPU_F18, offsetof(exc_frame_t, f18));
ASSYM(EXC_FPU_F19, offsetof(exc_frame_t, f19));
ASSYM(EXC_FPU_F20, offsetof(exc_frame_t, f20));
ASSYM(EXC_FPU_F21, offsetof(exc_frame_t, f21));
ASSYM(EXC_FPU_F22, offsetof(exc_frame_t, f22));
ASSYM(EXC_FPU_F23, offsetof(exc_frame_t, f23));
ASSYM(EXC_FPU_F24, offsetof(exc_frame_t, f24));
ASSYM(EXC_FPU_F25, offsetof(exc_frame_t, f25));
ASSYM(EXC_FPU_F26, offsetof(exc_frame_t, f26));
ASSYM(EXC_FPU_F27, offsetof(exc_frame_t, f27));
ASSYM(EXC_FPU_F28, offsetof(exc_frame_t, f28));
ASSYM(EXC_FPU_F29, offsetof(exc_frame_t, f29));
ASSYM(EXC_FPU_F30, offsetof(exc_frame_t, f30));
ASSYM(EXC_FPU_F31, offsetof(exc_frame_t, f31));
ASSYM(EXC_FPU_FSR, offsetof(exc_frame_t, fsr));

ASSYM(KERN_EXC_FRAME_SIZE, sizeof(cpu_exc_frame_t));
ASSYM(USER_EXC_FRAME_SIZE, sizeof(exc_frame_t));

ASSYM(P_USPACE, offsetof(proc_t, p_uspace));

ASSYM(PCPU_CURTHREAD, offsetof(pcpu_t, curthread));
ASSYM(PCPU_KSP, offsetof(pcpu_t, ksp));
