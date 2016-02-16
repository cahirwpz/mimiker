#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#define PAGESIZE 4096

/* Cache configuration. */
#define ILINE_SIZE 16 // L1 instruction cache line size
#define DLINE_SIZE 16 // L1 data cache line size
#define HCI 18        // Hardware Cache Initialization bit

/* Currently there is not much that can be configured. */
#define MHZ     200             /* CPU clock. */
#define TICKS_PER_MS (1000 * MHZ / 2)

#endif // GLOBAL_CONFIG_H
