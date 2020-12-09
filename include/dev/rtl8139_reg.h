#ifndef _RTL8139_REG_H_
#define _RTL8139_REG_H_
/*
 * Copied from FreeBSD: sys/dev/rl/if_rlreg.h
 */

#define RL_COMMAND 0x0037 /* command register */

#define RL_8139_CFG1 0x0052 /* config register #1 */

/*
 * Command register.
 */
#define RL_CMD_RESET 0x0010

/*
 * Config 1 register
 */
#define RL_CFG1_PWRON 0x00 /* power on */

#define RL_TIMEOUT (10 * 1000)
#endif /* !_RTL8139_REG_H_ */
