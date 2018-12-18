#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#define LINELEN 80

static int readline(char *line, size_t size) {
  char *s = fgets(line, size, stdin);
  if (!s)
    return 0;
  size_t n = strlen(s);
  if (line[n-1] == '\n')
    line[--n] = '\0';
  return 1;
}

static int do_exit(int argc, char *argv[], char *envp[]) {
  exit(EXIT_FAILURE);
}

static int do_env(int argc, char *argv[], char *envp[]) {
  while (*envp)
    puts(*envp++);
  return 0;
}

typedef int (*command_t)(int, char *[], char *[]);
typedef struct { char *name; command_t cmd; } builtin_t;

static const builtin_t builtins[] = {
  { "exit", do_exit },
  { "env", do_env },
  { NULL, NULL },
};

int main(int argc, char *argv[], char *envp[]) {
  char line[LINELEN];
  int status;

  while (1) {
    write(1, "# ", 2);
    if (!readline(line, LINELEN))
      break;

    /* Lookup builtin command */
    const builtin_t *b = NULL;
    for (b = builtins; b->name; b++)
      if (strcmp(line, b->name) == 0)
        break;
    if (b->name) {
      status = b->cmd(0, (char *[]){NULL}, envp);
      if (status)
        break;
      continue;
    }

    /* Process external command */
    pid_t pid;
    if ((pid = fork())) {
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

  return status;
}
