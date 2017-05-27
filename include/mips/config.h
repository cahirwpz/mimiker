#ifndef __MIPS_CONFIG_H__
#define __MIPS_CONFIG_H__

#define CPU_FREQ (100 * 1000 * 1000)
#define TICKS_PER_SEC CPU_FREQ
#define TICKS_PER_MS (CPU_FREQ / 1000)
#define TICKS_PER_US (CPU_FREQ / (1000 * 1000))

#endif
