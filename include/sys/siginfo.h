#ifndef _SYS_SIGINFO_H_
#define _SYS_SIGINFO_H_

#include <sys/sigtypes.h>

#ifdef _KERNEL
#include <sys/queue.h>
#include <string.h>
#endif /* !_KERNEL */

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

#ifdef _KERNEL

/* Kernel signal info.
 * Note that (for now) thread ID is unnecessary. */
typedef struct ksiginfo {
  u_long ksi_flags;
  TAILQ_ENTRY(ksiginfo) ksi_list;
  siginfo_t ksi_info;
} ksiginfo_t;

/* Queue of signals. */
typedef TAILQ_HEAD(ksiginfoq, ksiginfo) ksiginfoq_t;

/* Pending signals per thread. */
typedef struct sigpend {
  ksiginfoq_t sp_info;
  sigset_t sp_set;
} sigpend_t;

static inline void sigpend_init(sigpend_t *sp) {
  assert(sp != NULL);

  TAILQ_INIT(&sp->sp_info);
  __sigemptyset(&sp->sp_set);
}

void sigpend_destroy(sigpend_t *sp);

#define KSI_TRAP 0x01     /* signal caused by trap */
#define KSI_EMPTY 0x02    /* no additional information */
#define KSI_QUEUED 0x04   /* on a sigpend_t queue */
#define KSI_FROMPOOL 0x08 /* allocated from the ksiginfo pool */

/* Macros to initialize a ksiginfo_t. */
#define KSI_INIT(ksi)                                                          \
  do {                                                                         \
    memset((ksi), 0, sizeof(*(ksi)));                                          \
  } while (/*CONSTCOND*/ 0)

#define KSI_INIT_EMPTY(ksi)                                                    \
  do {                                                                         \
    KSI_INIT((ksi));                                                           \
    (ksi)->ksi_flags = KSI_EMPTY;                                              \
  } while (/*CONSTCOND*/ 0)

#define KSI_INIT_TRAP(ksi)                                                     \
  do {                                                                         \
    KSI_INIT((ksi));                                                           \
    (ksi)->ksi_flags = KSI_TRAP;                                               \
  } while (/*CONSTCOND*/ 0)

/* Copy the part of ksiginfo_t without the queue pointers */
#define KSI_COPY(fksi, tksi)                                                   \
  do {                                                                         \
    (tksi)->ksi_info = (fksi)->ksi_info;                                       \
    (tksi)->ksi_flags = (fksi)->ksi_flags;                                     \
  } while (/*CONSTCOND*/ 0)

/* Predicate macros to test how a ksiginfo_t was generated. */
#define KSI_TRAP_P(ksi) (((ksi)->ksi_flags & KSI_TRAP) != 0)
#define KSI_EMPTY_P(ksi) (((ksi)->ksi_flags & KSI_EMPTY) != 0)

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

#define CLD_EXITED 1
#define CLD_STOPPED 5
#define CLD_CONTINUED 6

#define SI_NOINFO 32767

#endif /* !_SYS_SIGINFO_H_ */
