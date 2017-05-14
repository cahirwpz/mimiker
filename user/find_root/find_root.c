/* This test dumps all files in kernel
 * the name find_root comes from executing 'find /' */

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/my_dirent.h>
#include <stdbool.h>

void recursive_dump(const char *dir_path) {
  char buf[256];
  char namebuf[256] = "";
  long basep = 0;
  int fd = open(dir_path, 0, O_RDONLY);

  int res = 0;
  struct dirent *dir;
  do {
    res = getdirentries(fd, buf, sizeof(buf), &basep);
    if(res < 0)
        return;
    dir = (struct dirent *)buf;
    while ((char *)dir < buf + res) {
      strcat(namebuf, dir_path);
      if (strcmp(dir_path, "/") != 0)
        strcat(namebuf, "/");
      strcat(namebuf, dir->d_name);
      printf("%s\n", namebuf);
      if (dir->d_type == DT_DIR 
         && strcmp(dir->d_name, ".")
         && strcmp(dir->d_name, ".."))
      {
        recursive_dump(namebuf);
      }
      namebuf[0] = '\0';
      dir = (struct dirent*)((char*)dir + dir->d_reclen);
    }
  } while (res > 0);
  close(fd);
}

int main(int argc, char **argv) {
  recursive_dump("/");
  return 0;
}
