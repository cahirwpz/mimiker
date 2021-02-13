#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#define THREAD_COUNT 3

static void work(int testnum) {
  while(1) {
      mkdir("/", 0);
      mkdir("/tmp/test", 0);
      mkdir("/tmp/test", 0);
      mkdir("/tmp//test///", 0);
      rmdir("/tmp/test");
      mkdir("/tmp/test", 0);

      mkdir("/tmp//test2///", 0);
      access("/tmp/test2", 0);

      mkdir("/tmp/test3", 0);
      mkdir("/tmp/test3/subdir1", 0);
      mkdir("/tmp/test3/subdir2", 0);
      mkdir("/tmp/test3/subdir3", 0);
      mkdir("/tmp/test3/subdir1", 0);
      access("/tmp/test3/subdir2", 0);

      mkdir("/tmp/test4/subdir4", 0);

      rmdir("/tmp/test/");
      rmdir("/tmp/test2");
      rmdir("/tmp/test3");
      rmdir("/tmp/test3/subdir1");
      rmdir("/tmp/test3/subdir2");
      rmdir("/tmp/test3/subdir3");
      rmdir("/tmp/test3");
      rmdir("/tmp/test3");
      rmdir("/tmp/test4/subdir4");

      mkdir("/tmp/test3/subdir1", 0);
  }
}

int main(int argc, char **argv) {
  for (int i = 0; i < THREAD_COUNT; i++) {
    if (fork() == 0)
      work(i);
  }

  return 0;
}
