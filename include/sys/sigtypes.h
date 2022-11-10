#ifndef _SYS_SIGTYPES_H_
#define _SYS_SIGTYPES_H_

/*
 * This header file defines various signal-related types.  We also keep
 * the macros to manipulate sigset_t here, to encapsulate knowledge of
 * its internals.
 */

#include <sys/types.h>

typedef int signo_t;

typedef struct {
  uint32_t __bits;
} sigset_t;

/*
 * Macro for manipulating signal masks.
 */
#define __sigmask(n) (1U << (((unsigned int)(n)-1) & 31))
#define __sigaddset(s, n) ((s)->__bits |= __sigmask(n))
#define __sigdelset(s, n) ((s)->__bits &= ~__sigmask(n))
#define __sigismember(s, n) (((s)->__bits & __sigmask(n)) != 0)
#define __sigemptyset(s) ((s)->__bits = 0x00000000)
#define __sigsetequal(s1, s2) ((s1)->__bits == (s2)->__bits)
#define __sigfillset(s) ((s)->__bits = 0xffffffff)
#define __sigplusset(s, t)                                                     \
  { (t)->__bits |= (s)->__bits; }
#define __sigminusset(s, t)                                                    \
  { (t)->__bits &= ~(s)->__bits; }
#define __sigandset(s, t)                                                      \
  { (t)->__bits &= (s)->__bits; }
#define __sigfindset(s) (__builtin_ffs((s)->__bits))

typedef struct sigaltstack {
  void *ss_sp;    /* signal stack base */
  size_t ss_size; /* signal stack length */
  int ss_flags;   /* SS_DISABLE and/or SS_ONSTACK */
} stack_t;

#endif /* !_SYS_SIGTYPES_H_ */
