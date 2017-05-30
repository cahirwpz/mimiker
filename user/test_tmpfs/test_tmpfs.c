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

void list_recursive(const char *dir_path) {
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

void create_write_remove()
{
  int fd = open("/tmp/some_file.c", O_RDWR | O_CREAT | O_TRUNC);

  printf("contents of tmp: \n");
  list_recursive("/tmp");

  char *str = "Hello ";
  write(fd, str, strlen(str));
  str = "everyone ";
  write(fd, str, strlen(str));
  str = "from mimiker!";
  write(fd, str, strlen(str) + 1);

  lseek(fd, 0, SEEK_SET);

  char buf[100];
  read(fd, buf, sizeof(buf));

  printf("%s\n", buf);

  unlink("/tmp/some_file.c");

  close(fd);

  printf("contents of tmp: \n");
  list_recursive("/tmp");
}

int main(int argc, char **argv) {
  create_write_remove();
}
