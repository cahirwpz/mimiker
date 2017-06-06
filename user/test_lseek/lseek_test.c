/* Userspace program testing lseek */

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>

static void test_set(const char *dir_path) {
  int fd = open(dir_path, 0, O_RDONLY);
  const int size=256;
  char buf[size+1];
  read(fd,buf,size);
  lseek(fd,0,SEEK_SET);
  char  buf2[size+1];
  read(fd,buf2,size);
  assert(strcmp(buf,buf2)==0);
  close(fd);
}

int main(int argc, char **argv) {
  const char *mypath="/bin/lseek_test";
  test_set(argc>1?argv[1]:mypath);
  return 0;
}
