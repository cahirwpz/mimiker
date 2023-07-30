#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/exec.h>
#include <sys/kenv.h>
#include <sys/ktest.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/wait.h>

#define UTEST_PATH "/bin/utest"

static u_int utest_seed = 0;

static __noreturn void utest_thread(void *arg) {
  proc_t *p = proc_self();
  /* Run user tests in a separate session. */
  int error = session_enter(p);
  assert(error == 0);
  char seed[32];
  snprintf(seed, sizeof(seed), "seed=%d", utest_seed);
  kern_execve(UTEST_PATH, (char *[]){UTEST_PATH, arg, NULL},
              (char *[]){seed, NULL});
}

/* This is the klog mask used with utests. */
#define KL_UTEST_MASK                                                          \
  (KL_ALL & (~(KL_MASK(KL_INTR) | KL_MASK(KL_PMAP) | KL_MASK(KL_PHYSMEM))))

static int test_user(void) {
  const char *utest_mask = kenv_get("klog-utest-mask");
  unsigned new_klog_mask =
    utest_mask ? (unsigned)strtol(utest_mask, NULL, 16) : KL_UTEST_MASK;
  unsigned old_klog_mask = klog_setmask(new_klog_mask);

  static int first_call = 1;

  if (first_call) {
    const char *seed_str = kenv_get("seed");
    if (seed_str)
      utest_seed = strtoul(seed_str, NULL, 10);
    first_call = 0;
  } else {
    utest_seed = rand_r(&utest_seed) & INT32_MAX;
  }

  pid_t cpid;
  if (do_fork(utest_thread, "all", &cpid))
    panic("Could not start test!");

  int status;
  pid_t pid;
  do_waitpid(cpid, &status, 0, &pid);
  assert(cpid == pid);

  /* Restore previous klog mask */
  klog_setmask(old_klog_mask);

  return status ? KTEST_FAILURE : KTEST_SUCCESS;
}

KTEST_ADD(utest, test_user, 0);
