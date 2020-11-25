#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv) {
  if (argc < 4)
    return 1;

  char *name = argv[1];
  char *group = argv[2];
  char *file = argv[3];

  struct passwd *pwd = getpwnam(name);
  struct group *gr = getgrnam(group);

  uid_t uid = -1;
  gid_t gid = -1;

  if (pwd != NULL)
    uid = pwd->pw_uid;

  if (gr != NULL) 
    gid = gr->gr_gid;

  printf("setting %s to %d %d\n", file, uid, gid);

  int r = chown(file, uid, gid);

  if (r != 0)
    fprintf(stderr, "chown: %s\n", strerror(errno));

  return r;
}

