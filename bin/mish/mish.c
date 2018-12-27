#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#define LINELEN 80
#define NARGS 10

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
  char *new_argv[NARGS+1];
  int status;

  while (1) {
    write(1, "# ", 2);
    if (!readline(line, LINELEN))
      break;

    int new_argc;
    char *brkt, *word;
    for (new_argc = 0, word = strtok_r(line, " ", &brkt);
         new_argc < NARGS && word;
         new_argc++, word = strtok_r(NULL, " ", &brkt))
    {
      new_argv[new_argc] = word;
    }
    new_argv[new_argc] = NULL;

    /* Lookup builtin command */
    const builtin_t *b = NULL;
    for (b = builtins; b->name; b++)
      if (strcmp(new_argv[0], b->name) == 0)
        break;
    if (b->name) {
      status = b->cmd(new_argc, new_argv, envp);
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
      execve(new_argv[0], new_argv, envp);
      perror("execve failed");
      return EXIT_FAILURE;
    }
  }

  return status;
}
