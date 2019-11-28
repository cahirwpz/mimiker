#ifndef _SYS_PRIORITY_H_
#define _SYS_PRIORITY_H_

#include <stdint.h>

/* Priority range spans from 0 (the highest) to 255 (the lowest) */
typedef uint8_t prio_t;

#ifdef _KERNEL

#define PRIO_QTY 256

/* Quantity of interrupt threads, kernel threads and user threads.
   These three numbers must sum up to PRIO_QTY! */
#define PRIO_ITHRD_QTY 32
#define PRIO_KTHRD_QTY 64
#define PRIO_UTHRD_QTY 160

/* The following functions transform [0, PRIO_QTY - 1] range to the range of
   priorities of ithreads, kthreads or uthreads. E.g. prio_kthread(0) returns
   highest kernel thread priority (currently: 32), prio_kthread(PRIO_QTY - 1)
   returns lowest kernel thread priority (currently: 95). */
static inline prio_t prio_ithread(int n) {
  return n * (PRIO_ITHRD_QTY - 1) / (PRIO_QTY - 1);
}

static inline prio_t prio_kthread(int n) {
  return PRIO_ITHRD_QTY + n * (PRIO_KTHRD_QTY - 1) / (PRIO_QTY - 1);
}

static inline prio_t prio_uthread(int n) {
  return PRIO_ITHRD_QTY + PRIO_KTHRD_QTY +
         n * (PRIO_UTHRD_QTY - 1) / (PRIO_QTY - 1);
}

/* You must use following functions to compare two priorities. */
static inline bool prio_le(prio_t p1, prio_t p2) {
  return p1 >= p2;
}

static inline bool prio_lt(prio_t p1, prio_t p2) {
  return p1 > p2;
}

static inline bool prio_ge(prio_t p1, prio_t p2) {
  return p1 <= p2;
}

static inline bool prio_gt(prio_t p1, prio_t p2) {
  return p1 < p2;
}

static inline bool prio_ne(prio_t p1, prio_t p2) {
  return p1 != p2;
}

static inline bool prio_eq(prio_t p1, prio_t p2) {
  return p1 == p2;
}

#endif /* !_KERNEL */

#endif /* !_SYS_PRIORITY_H_ */
