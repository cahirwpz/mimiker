#include <stdio.h>

/*
 * in order to test passing parameters to user program
 * run launcher with any of the following commands:
 *
 *./launch -t klog-quiet=1 init=\"/bin/argsenv test00  \\w ala ma kota\" envv=\"\" param
 *./launch -t klog-quiet=1 init=\"/bin/argsenv  test01 ab= \\-c envv=\" param  envv=\"PATH=/bin\\bin/  P=NP\"
 *./launch -t klog-quiet=1 init=\" /bin/argsenv test02\" envv=\"\"
 *./launch -t klog-quiet=1 envv=\"abcd= 3=efgh L4\" param init=\"/bin/argsenv test03 ./ \\n- w = _+- \" 
 *./launch -t klog-quiet=1 envv=\" _NULL\" init=\"/bin/argsenv test04 b=b!=a=b 0\"
 *
 */

#define N_TESTS 5

char **testar[] = {
  (char *[]) {"/bin/argsenv", "test00", "\\w", "ala", "ma", "kota", NULL},
  (char *[]) {"/bin/argsenv", "test01", "ab=", "\\-c", "envv=", NULL},
  (char *[]) {"/bin/argsenv", "test02", NULL},
  (char *[]) {"/bin/argsenv", "test03", "./", "\\n-", "w", "=", "_+-", NULL},
  (char *[]) {"/bin/argsenv", "test04", "b=b!=a=b", "0", NULL}
};

char **testen[] = {
  (char *[]) {NULL},
  (char *[]) {"PATH=/bin\\bin/", "P=NP", NULL},
  (char *[]) {NULL},
  (char *[]) {"abcd=", "3=efgh", "L4", NULL},
  (char *[]) {"_NULL", NULL}
};
  
int strcmp(char *a, char *b)
{
  while (*a && *b && *a == *b)
    a++, b++;
  return *a == *b ? 0 : -1;
}

int test(char** a, char** b)
{
  while (*a && *b)
    if (strcmp(*a++, *b++))
      return -1;
  return (*a || *b) ? -1 : 0;
}

int main(int argc, char **argv, char **envv) {
  printf("This is argsenv program. You can use it for testing and "
         "experiments in userspace. Don't commit any useful kernel tests here, "
         "please place them in utest program.\n");

  printf("argc is: %d\n", argc);
  for (int i = 0; i < argc; i++)
    printf("argv[%d] is: '%s'\n", i, argv[i]);

  printf("envv is:\n");
  for (char **p = envv; *p; p++)
    printf("%s\n", *p);

  if (argc == 1)
    return 0;
  
  for (int i = 0; i < N_TESTS; i++){
    if (strcmp(argv[1], testar[i][1]))
      continue;
    printf("\nRunning test %.02d:\n", i);
    
    if (test(argv, testar[i])){
      printf("Error while passing init.\n");
      return 1;
    }

    if (test(envv, testen[i])){
      printf("Error while passing envv.\n");
      return 1;
    }

    printf("Test passed.\n");
    return 0;
  }

  printf("No test provided.\n");
  return 0;
}

