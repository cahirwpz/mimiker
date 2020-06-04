#include <sys/wait.h>
#include <sys/resource.h>

pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage) {
  int rv = _wait4(pid, status, options, (struct krusage *)rusage);
  if (rusage) {
    bt2tv((bintime_t *)&rusage->ru_utime, &rusage->ru_utime);
    bt2tv((bintime_t *)&rusage->ru_stime, &rusage->ru_stime);
  }
  return rv;
}