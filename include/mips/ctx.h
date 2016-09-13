#ifndef __MIPS_CTX_H__
#define __MIPS_CTX_H__

#include <mips/asm.h>

/* Register order in processor context structure. */
#define REG_AT           0
#define REG_V0           1
#define REG_V1           2
#define REG_A0           3
#define REG_A1           4
#define REG_A2           5
#define REG_A3           6
#define REG_T0           7
#define REG_T1           8
#define REG_T2           9
#define REG_T3          10
#define REG_T4          11
#define REG_T5          12
#define REG_T6          13
#define REG_T7          14
#define REG_S0          15
#define REG_S1          16
#define REG_S2          17
#define REG_S3          18
#define REG_S4          19
#define REG_S5          20
#define REG_S6          21
#define REG_S7          22
#define REG_T8          23
#define REG_T9          24
#define REG_GP          25
#define REG_SP          26
#define REG_FP          27
#define REG_RA          28
#define REG_LO          29
#define REG_HI          30
#define REG_PC          31
#define REG_SR          32
#define REG_BADVADDR    33
#define REG_CAUSE       34
#define REG_TCB         35 /* current TCB, stored in C0_USERLOCAL */
#define REG_NUM         36 /* number of registers in context structure */

#define EXC_FRAME_SIZE (REG_NUM * SZREG)

#define SAVE_REG(reg, offset, base) \
        sw      reg, (SZREG * REG_ ## offset)(base)

#define LOAD_REG(reg, offset, base) \
        lw      reg, (SZREG * REG_ ## offset)(base)

#endif
