#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#define LINELEN 80

int main(int argc, char *argv[], char *envp[]) {
  char line[LINELEN];
  while (1) {
    write(1, "# ", 2);
    char *s = fgets(line, LINELEN, stdin);
    if (!s)
      break;

    size_t n = strlen(s);
    if (line[n-1] == '\n')
      line[--n] = '\0';

    if (strcmp(s, "exit") == 0)
      break;

    pid_t pid;
    if ((pid = fork())) {
      int status;
      waitpid(pid, &status, 0);
      if (status)
        printf("Child exited with %d\n", status);
    } else {
      fprintf(stderr, "run %s\n", line);
      execve(line, (char *[]){line, NULL}, envp);
      perror("execve failed");
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
