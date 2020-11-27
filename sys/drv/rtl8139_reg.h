/*
 * Copied from FreeBSD: sys/dev/rl/if_rlreg.h
 */

#define RL_COMMAND              0x0037          /* command register */
#define RL_TXCFG                0x0040          /* transmit config */

#define RL_8139_CFG1            0x0052          /* config register #1 */

#define RL_HWREV_8139CPLUS      0x74800000

/*
 * Command register.
 */
#define RL_CMD_RESET            0x0010

/*
 * Config 1 register
 */
#define RL_CFG1_PWRON           0x00

#define RL_TIMEOUT              (10*1000)
