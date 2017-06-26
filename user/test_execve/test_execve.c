#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>


/*All envp arrays will be empty. Environments are not yet implemented (?)*/

void execve_success_test(const char path[], char *const argv[]) {

  char * const envp[] = {NULL};  
  execve(path,argv,envp); 
}


/******************************************************************************/
/* These tests should fail with error codes given in the table below.         */
/* These tests are by no way complete.*/
/******************************************************************************/

void execve_failure_test() {


}




int main() {

/******************************************************************************/
/* These tests should succeed, i.e dump_args should report correct arguments  */
/******************************************************************************/
   /* This always ends in kernel panic due to exhausted memory*/
   /* execve_success_test("/bin/test_execve",(char *const[]){NULL}); */

  /* This runs ls_rec*/
   /* execve_success_test("/bin/ls_rec",(char *const[]){NULL}); */
  
  /* The tests below run dump_args with various parameters.*/

  /* execve_success_test("/bin/dump_args",(char *const[]){NULL}); */
  /* execve_success_test("/bin/dump_args",(char *const[]){"FIRST"}); */
  /* execve_success_test("/bin/dump_args",(char *const[]){"FIRST", "SECOND", NULL}); */
  /* execve_success_test("/bin/dump_args",(char *const[]){"FIRST", "SECOND", "THIRD", NULL}); */


  /*
    Why the value below gives kernel panic? 
    #define ARG_MAX (256*1024)
  */

  /*
    Why ARG_MAX = 256*4 gives errorneous output without noticing?
    Probable reason: unnoticed failures of kmalloc's
  */
/* #define ARG_MAX (256*2)   */

/*   char* argv[(ARG_MAX / 3) + 1]; */

/*   for(int i = 0; i < ARG_MAX / 3; i++) { */
/*     argv[i] = malloc(3); */
/*     sprintf(argv[i], "%d", i % 100); */
/*     printf("%s\n", argv[i]); */
/*   }  */

/*   argv[ARG_MAX / 3] = NULL;   */
/*   execve_success_test("/bin/dump_args",argv); */


  
  printf("Failure! This statement should be unreachable.\n");

  return 0;
}
