#include <stdio.h>
#include <assert.h>

int test_procstat(void) {
  int euid, pid, ppid, pgrp, session, got;
  char state;
#define PROC_COMM_MAX 128
  char command[PROC_COMM_MAX];

  FILE *pstat = fopen("/dev/procstat", "r");
  assert(pstat != NULL);

  for(;;) {
    if ((got = fscanf(pstat, "%d\t%d\t%d\t%d\t%d\t%c\t", &euid, &pid, &ppid,
                      &pgrp, &session, &state)) == -1)
      break;

    if (got < 6)
      return 1;

    if ((fgets(command, PROC_COMM_MAX, pstat)) == NULL)
      return 1;
  }
  return 0;
}
