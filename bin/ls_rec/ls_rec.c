/* List recursively files starting from given directory. It performs similar
 * action to 'find $path' and can be compiled on other unix-like OSes. */

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>

static const char *filetype[] = {[DT_UNKNOWN] = "???", [DT_FIFO] = "fifo",
                                 [DT_CHR] = "chr",     [DT_DIR] = "dir",
                                 [DT_BLK] = "blk",     [DT_REG] = "file",
                                 [DT_LNK] = "link",    [DT_SOCK] = "sock"};

static void list_recursive(const char *dir_path) {
  char buf[256];
  char namebuf[256] = "";
  long basep = 0;
  int fd = open(dir_path, 0, O_RDONLY);

  int res = 0;
  struct dirent *dir;
  do {
    res = getdirentries(fd, buf, sizeof(buf), &basep);
    if (res < 0)
      return;
    dir = (struct dirent *)buf;
    while ((char *)dir < buf + res) {
      strcat(namebuf, dir_path);
      if (strcmp(dir_path, "/") != 0)
        strcat(namebuf, "/");
      strcat(namebuf, dir->d_name);
      printf("[%4s, ino=%d] %s\n", filetype[dir->d_type], dir->d_fileno,
             namebuf);
      if (dir->d_type == DT_DIR && strcmp(dir->d_name, ".") &&
          strcmp(dir->d_name, "..")) {
        list_recursive(namebuf);
      }
      namebuf[0] = '\0';
      dir = (struct dirent *)((char *)dir + dir->d_reclen);
    }
  } while (res > 0);
  close(fd);
}

int main(int argc, char **argv) {
  list_recursive(argc > 1 ? argv[1] : "/");
  return 0;
}
