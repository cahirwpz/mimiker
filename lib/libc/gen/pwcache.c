#include <pwd.h>
#include <grp.h>

const char *user_from_uid(uid_t uid, int noname) {
  struct passwd *pw;
  pw = getpwuid(uid);
  if (pw == NULL)
    return NULL;

  return pw->pw_name;
}

const char *group_from_gid(gid_t gid, int noname) {
  struct group *gr;
  gr = getgrgid(gid);
  if (gr == NULL)
    return NULL;

  return gr->gr_name;
}

int uid_from_user(const char *name, uid_t *uid) {
  struct passwd *pw;
  pw = getpwnam(name);
  if (pw == NULL)
    return -1;

  *uid = pw->pw_uid;
  return 0;
}

int gid_from_group(const char *name, gid_t *gid) {
  struct group *gr;
  gr = getgrnam(name);
  if (gr == NULL)
    return -1;

  *gid = gr->gr_gid;
  return 0;
}
