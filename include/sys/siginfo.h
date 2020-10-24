#ifndef _SYS_SIGINFO_H_
#define _SYS_SIGINFO_H_

#include <sys/sigtypes.h>

typedef struct siginfo {
  int si_signo;
  int si_code;
  int si_errno;
  union {
    /* child */
    struct {
      pid_t si_pid;
      uid_t si_uid; /* effective user id */
    };
    /* fault */
    struct {
      void *si_addr;
      int si_trap;
      int si_trap2;
      int si_trap3;
    };
  };
} siginfo_t;

#define CLD_EXITED 1
#define CLD_STOPPED 5
#define CLD_CONTINUED 6

#define SI_NOINFO 32767

#ifdef _KERNEL
#include <sys/queue.h>
#include <sys/libkern.h>

typedef enum {
  KSI_TRAP = 1,    /* signal caused by trap */
  KSI_RAW = 2,     /* no additional information for signal */
  KSI_QUEUED = 4,  /* on a sigpend_t queue */
  KSI_FROMPOOL = 8 /* allocated from the ksiginfo pool */
} ksi_flags_t;

/* Kernel signal info.
 * Note that (for now) thread ID is unnecessary. */
typedef struct ksiginfo {
  TAILQ_ENTRY(ksiginfo) ksi_list;
  siginfo_t ksi_info;
  ksi_flags_t ksi_flags;
} ksiginfo_t;

/* Queue of signals. */
typedef TAILQ_HEAD(ksiginfoq, ksiginfo) ksiginfoq_t;

/* Pending signals per thread. */
typedef struct sigpend {
  ksiginfoq_t sp_info;
  sigset_t sp_set;
} sigpend_t;

void sigpend_init(sigpend_t *sp);
void sigpend_destroy(sigpend_t *sp);

#define DEF_KSI_TRAP(sig)                                                      \
  (ksiginfo_t) {                                                               \
    .ksi_flags = KSI_TRAP, .ksi_signo = (sig)                                  \
  }

#define DEF_KSI_RAW(sig)                                                       \
  (ksiginfo_t) {                                                               \
    .ksi_flags = KSI_RAW, .ksi_signo = (sig)                                   \
  }

/* Field access macros */
#define ksi_signo ksi_info.si_signo
#define ksi_code ksi_info.si_code
#define ksi_errno ksi_info.si_errno

#define ksi_pid ksi_info.si_pid
#define ksi_uid ksi_info.si_uid

#define ksi_addr ksi_info.si_addr
#define ksi_trap ksi_info.si_trap
#define ksi_trap2 ksi_info.si_trap2
#define ksi_trap3 ksi_info.si_trap3

#endif /* !_KERNEL */

#endif /* !_SYS_SIGINFO_H_ */
