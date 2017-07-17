#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include <sys/hash.h>


/*A simple hash function stolen from somewhere...*/
/* uint32_t jenkins_one_at_a_time_hash(char *key, size_t len) { */
/*   uint32_t hash, i; */
/*   for (hash = i = 0; i < len; ++i) { */
/*     hash += key[i]; */
/*     hash += (hash << 10); */
/*     hash ^= (hash >> 6); */
/*   } */
/*   hash += (hash << 3); */
/*   hash ^= (hash >> 11); */
/*   hash += (hash << 15); */
/*   return hash; */
/* } */

/* /\*...and its single-step version.*\/ */
/* uint32_t jenkins_one_step(uint32_t hash, char key) { */

/*   hash += key; */
/*   hash += (hash << 10); */
/*   hash ^= (hash >> 6); */

/*   return hash; */
/* } */

/* uint32_t jenkins_final(uint32_t hash) { */

/*   hash += (hash << 3); */
/*   hash ^= (hash >> 11); */
/*   hash += (hash << 15); */

/*   return hash; */
/* } */

int runexecve(const char *path, char *const argv[]) {

  char *const envp[] = {NULL};

  return execve(path, argv, envp);
}

#define PATH_MAX 1024
#define ARG_MAX 256

/* Since Newlib's errno.h is incompatible with kernel's errno.h we need
   to define some symbols in place*/

#undef ENAMETOOLONG
#undef E2BIG

#define ENAMETOOLONG 63 /* File name too long */
#define E2BIG 7         /* Argument list too long */

#define assert_fail(expr, err) assert(expr == -1 && errno == err)

int test_execve_errors(void) {

  char *const valid_path = "/bin/dump_args";
  char too_long_path[PATH_MAX + 1] = {[0 ... PATH_MAX - 1] = 'A',
                                      [PATH_MAX] = '\0'};

  char *const valid_argv[] = {valid_path, NULL};
  char *const non_terminated_argv[] = {valid_path};
  char *const too_long_argv[ARG_MAX + 1] = {[0 ... ARG_MAX - 1] = "A",
                                            [ARG_MAX] = NULL};

  char arg_too_long[ARG_MAX + 1] = {[0 ... ARG_MAX - 1] = 'A',
                                    [ARG_MAX] = '\0'};

  char arg_half_max[] = {[0 ...(ARG_MAX - 1) / 2] = 'A', [ARG_MAX] = '\0'};

  char *const argv_with_arg_too_long[2] = {arg_too_long, NULL};
  char *const argv_with_args_too_long[3] = {arg_half_max, arg_half_max, NULL};

  assert_fail(runexecve(too_long_path, valid_argv), ENAMETOOLONG);
  assert_fail(runexecve(too_long_path, non_terminated_argv), ENAMETOOLONG);

  assert_fail(runexecve(valid_path, too_long_argv), E2BIG);
  assert_fail(runexecve(valid_path, non_terminated_argv), E2BIG);

  assert_fail(runexecve(valid_path, argv_with_arg_too_long), E2BIG);
  assert_fail(runexecve(valid_path, argv_with_args_too_long), E2BIG);

  return 0;
}

short hashed_execve_test(char *const argv[]) {

  int argc;
  int hash = HASH32_STR_INIT;

  for (argc = 0; argv[argc] != NULL; argc++)
    ;

  for (int i = 0; i < argc; i++) {
    /* for (unsigned int j = 0; j < strlen(argv[i]); j++) */
    /*   hash = jenkins_one_step(hash, argv[i][j]); */

    hash = hash32_str(argv[i], hash);
    
  }

  /*  hash = jenkins_final(hash) & 255;*/
  hash &= 255;

  pid_t n = fork();

  if (n == 0) { /*We're in a child.*/

    runexecve("/bin/hash_args", argv);

  } else { /*We're in the parent*/

    int status;
    pid_t rpid = wait(&status);

    assert(rpid == n);

    return (WEXITSTATUS(status) & 255) == hash;
  }

  __builtin_unreachable();
}

int test_execve_basic(void) {

  char *const argv1[] = {"Ala", NULL};
  char *const argv2[] = {"Ala", "ma", NULL};
  char *const argv3[] = {"Ala", "ma", "kota", NULL};
  char *const argv4[] = {"Ala", "ma", "kota", "psa", NULL};

  assert(hashed_execve_test(argv1));
  assert(hashed_execve_test(argv2));
  assert(hashed_execve_test(argv3));
  assert(hashed_execve_test(argv4));

  return 0;
}
