#include <stdio.h>
#include <unistd.h>

int main(){
  printf("Fork test.\n");

  int n = fork();
  if(n == 0){
    printf("This is child!\n");
  }else{
    printf("This is parent! (%d)\n", n);
  }
  
  return 0;
}
