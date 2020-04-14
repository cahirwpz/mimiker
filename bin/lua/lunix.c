/* Adapted from:
 * https://github.com/JakubSzczerbinski/luash/blob/master/syscalls.c */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "lua.h"
#include "lauxlib.h"

#define table_push(type, L, s, f)                                              \
  lua_push##type(L, (s)->f);                                                   \
  lua_setfield(L, -2, #f)

#define table_pushinteger(L, s, f) table_push(integer, L, s, f)
#define table_pushstring(L, s, f) table_push(string, L, s, f)

static void lua_pusherrno(lua_State *L) {
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, errno);
  lua_setfield(L, -2, "errno");
  lua_pushstring(L, strerror(errno));
  lua_setfield(L, -2, "msg");
}

#define error_if(cond) if (cond) throw_errno(L)

static void throw_errno(lua_State *L) {
  lua_pusherrno(L);
  lua_error(L);
}

/* copied over from lauxlib.c */
static int type_error(lua_State *L, int arg, int tag) {
  const char *msg;
  const char *typearg; /* name for the type of the actual argument */
  const char *typename = lua_typename(L, tag);
  if (luaL_getmetafield(L, arg, "__name") == LUA_TSTRING)
    typearg = lua_tostring(L, -1); /* use the given type name */
  else if (lua_type(L, arg) == LUA_TLIGHTUSERDATA)
    typearg = "light userdata"; /* special name for messages */
  else
    typearg = luaL_typename(L, arg); /* standard name */
  msg = lua_pushfstring(L, "%s expected, got %s", typename, typearg);
  return luaL_argerror(L, arg, msg);
}

/* fork() -> (pid: integer) or nil */
static int unix_fork(lua_State *L) {
  pid_t pid = fork();
  error_if(pid == -1);
  if (pid == 0) return 0;
  lua_pushinteger(L, pid);
  return 1;
}

static void check_table_string(lua_State *L, int index) {
  if (!lua_istable(L, index))
    type_error(L, index, LUA_TTABLE);

  lua_pushvalue(L, index);
  for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
    (void)luaL_checkstring(L, -1);
  lua_pop(L, 1);
}

static char **make_string_vector(lua_State *L, int index) {
  char **strv = malloc(sizeof(char *) * (luaL_len(L, index) + 1));
  int i;

  lua_pushvalue(L, index);
  for (i = 0, lua_pushnil(L); lua_next(L, -2); i++, lua_pop(L, 1)) {
    /* It's safe to call lua_tolstring as we know the value is string. */
    size_t len;
    const char *str = lua_tolstring(L, -1, &len);
    strv[i] = malloc(len + 1);
    strcpy(strv[i], str);
  }
  lua_pop(L, 1);

  strv[i] = NULL;
  return strv;
}

static void free_string_vector(char **strv) {
  for (int i = 0; strv[i]; i++)
    free(strv[i]);
  free(strv);
}

extern char **environ;

/* execve(path:string, argv: table(string), envp: table(string) or nil)
 * -> error or noreturn */
static int unix_execve(lua_State *L) {
  /* Before any memory allocation is made, make sure all arguments are
   * convertible to C values. */
  const char *path = luaL_checkstring(L, 1);
  bool use_environ;

  check_table_string(L, 2);
  use_environ = lua_isnoneornil(L, 3);
  if (!use_environ)
    check_table_string(L, 3);

  /* Now it's safe to convert tables to C values. */
  char **argv = make_string_vector(L, 2);
  char **envp = use_environ ? environ : make_string_vector(L, 3);

  execve(path, argv, envp);

  /* Free argv & envp if execve failed. */
  if (use_environ)
    free_string_vector(envp);
  free_string_vector(argv);

  lua_pusherrno(L);
  return 1;
}

/* waitpid(pid: integer, options: integer)
 * -> (pid: integer, status: integer) */
static int unix_waitpid(lua_State *L) {
  pid_t pid = luaL_checkinteger(L, 1);
  int wstatus;
  int options = luaL_checkinteger(L, 2);
  int result = waitpid(pid, &wstatus, options);
  error_if(result == -1);
  lua_pushinteger(L, result);
  lua_pushinteger(L, wstatus);
  return 2;
}

/* pipe() -> (fd_1: integer, fd_2: integer) */
static int unix_pipe(lua_State *L) {
  int fd[2];
  int result = pipe(fd);
  error_if(result == -1);
  lua_pushinteger(L, fd[0]);
  lua_pushinteger(L, fd[1]);
  return 2;
}

/* open(path: string, flags: integer or nil, mode: integer or nil)
 * -> (fd: integer) */
static int unix_open(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int flags = luaL_optinteger(L, 2, 0);
  int mode = luaL_optinteger(L, 3, 0);
  int fd = open(path, flags, mode);
  error_if(fd == -1);
  lua_pushinteger(L, fd);
  return 1;
}

/* close(fd: integer) -> nil */
static int unix_close(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int result = close(fd);
  error_if(result == -1);
  return 0;
}

/* read(fd: integer, size: integer) -> string or nil */
static int unix_read(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int size = luaL_checkinteger(L, 2);
  char *buf = malloc(size);
  error_if(buf == NULL);
  int result = read(fd, buf, size);
  error_if(result == -1);
  if (result == 0)
    return 0;
  lua_pushlstring(L, buf, result);
  return 1;
}

/* write(fd: integer, buf: string, size: integer or nil)
 * -> (written: integer) */
static int unix_write(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  size_t buflen;
  const char *buf = luaL_checklstring(L, 2, &buflen);
  size_t size = luaL_optinteger(L, 3, buflen);
  int result = write(fd, buf, (buflen < size) ? buflen : size);
  error_if(result == -1);
  lua_pushinteger(L, result);
  return 1;
}

/* lseek(fd: integer, offset: integer, whence: integer) -> (pos: integer) */
static int unix_lseek(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  off_t offset = luaL_checkinteger(L, 2);
  int whence = luaL_checkinteger(L, 3);
  off_t result = lseek(fd, offset, whence);
  error_if(result == -1);
  lua_pushinteger(L, result);
  return 1;
}

static int make_stat_table(lua_State *L, struct stat *buf) {
  lua_createtable(L, 0, 10);
  table_pushinteger(L, buf, st_dev);
  table_pushinteger(L, buf, st_ino);
  table_pushinteger(L, buf, st_mode);
  table_pushinteger(L, buf, st_nlink);
  table_pushinteger(L, buf, st_uid);
  table_pushinteger(L, buf, st_gid);
  table_pushinteger(L, buf, st_rdev);
  table_pushinteger(L, buf, st_size);
  table_pushinteger(L, buf, st_blksize);
  table_pushinteger(L, buf, st_blocks);
  return 1;
}

/* stat(path: string) | fstat(fd: integer) ->
 * {st_dev: integer, st_ino: integer, st_mode: integer, st_nlink: integer,
 *  st_uid: integer, st_gid: integer, st_rdev: integer, st_size: integer,
 *  st_blksize: integer, st_blocks: integer}
 *
 * TODO st_atimespec, st_mtimespec, st_ctimespec */
static int unix_stat(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  struct stat buf;
  int result = stat(path, &buf);
  error_if(result == -1);
  return make_stat_table(L, &buf);
}

static int unix_fstat(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  struct stat buf;
  int result = fstat(fd, &buf);
  error_if(result == -1);
  return make_stat_table(L, &buf);
}

/* dup(fd: integer) -> (fd: integer) */
static int unix_dup(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int fd2 = dup(fd);
  error_if(fd2 == -1);
  lua_pushinteger(L, fd2);
  return 1;
}

/* dup(fd_1: integer, fd_2: integer) -> (fd_2: integer) */
static int unix_dup2(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int fd2 = luaL_checkinteger(L, 2);
  int result = dup2(fd, fd2);
  error_if(result == -1);
  return 0;
}

/* exit(status: integer) -> noreturn */
static int unix_exit(lua_State *L) {
  int status = luaL_checkinteger(L, 1);
  exit(status);
  return 0;
}

/* getcwd() -> (path: string) */
static int unix_getcwd(lua_State *L) {
  char *buf = malloc(PATH_MAX);
  error_if(buf == NULL);
  getcwd(buf, PATH_MAX);
  lua_pushstring(L, buf);
  free(buf);
  return 1;
}

/* chdir(path: string) -> nil */
static int unix_chdir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int result = chdir(path);
  error_if(result == -1);
  return 0;
}

#define DIRENT_BUFLEN 4096
typedef struct dirent dirent_t;

/* getdents(fd: integer)
 * -> table({d_type: integer, d_fileno: integer, d_name: string}) */
static int unix_getdents(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  char *buf = malloc(DIRENT_BUFLEN);
  error_if(buf == NULL);
  size_t nread = 0;
  lua_createtable(L, 0, 0);
  int i = 1;
  do {
    nread = getdents(fd, buf, DIRENT_BUFLEN);
    for (dirent_t *dir = (dirent_t *)buf; (char *)dir < buf + nread;
         dir = (dirent_t *)((char *)dir + dir->d_reclen)) {
      lua_pushinteger(L, i++);
      lua_createtable(L, 0, 3);
      table_pushinteger(L, dir, d_type);
      table_pushinteger(L, dir, d_fileno);
      table_pushstring(L, dir, d_name);
      lua_settable(L, -3);
    }
  } while (nread);
  free(buf);
  return 1;
}

/* getpid() -> (pid: integer) */
static int unix_getpid(lua_State *L) {
  lua_pushinteger(L, getpid());
  return 1;
}

/* getppid() -> (ppid: integer) */
static int unix_getppid(lua_State *L) {
  lua_pushinteger(L, getppid());
  return 1;
}

/* kill(pid: integer, signum: integer) -> nil */
static int unix_kill(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  int signum = luaL_checkinteger(L, 2);
  int result = kill(pid, signum);
  error_if(result == -1);
  return 0;
}

/* killpg(pgid: integer, signum: integer) -> nil */
static int unix_killpg(lua_State *L) {
  int pgid = luaL_checkinteger(L, 1);
  int signum = luaL_checkinteger(L, 2);
  int result = killpg(pgid, signum);
  error_if(result == -1);
  return 0;
}

/* setpgid(pid: integer, pgid: integer) -> nil */
static int unix_setpgid(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  int pgid = luaL_checkinteger(L, 2);
  int result = setpgid(pid, pgid);
  error_if(result == -1);
  return 0;
}

/* getpgid(pid: integer) -> (pgid: integer) */
static int unix_getpgid(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  int result = getpgid(pid);
  error_if(result == -1);
  lua_pushinteger(L, result);
  return 1;
}

/* unlink(path: string) -> nil */
static int unix_unlink(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int result = unlink(path);
  error_if(result == -1);
  return 0;
}

/* rmdir(path: string) -> nil */
static int unix_rmdir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int result = rmdir(path);
  error_if(result == -1);
  return 0;
}

/* access(path: string, mode: integer) -> nil */
static int unix_access(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int mode = luaL_checkinteger(L, 2);
  int result = access(path, mode);
  error_if(result == -1);
  return 0;
}

/* Lua module construction code follows. */
typedef struct {
  const char *name;
  const int value;
} named_int_t;

/* clang-format off */
static const luaL_Reg tab_funcs[] = {
  {"getcwd", unix_getcwd},
  {"chdir", unix_chdir},
  {"execve", unix_execve},
  {"fork", unix_fork},
  {"waitpid", unix_waitpid},
  {"pipe", unix_pipe},
  {"dup", unix_dup},
  {"close", unix_close},
  {"dup2", unix_dup2},
  {"exit", unix_exit},
  {"getdents", unix_getdents},
  {"open", unix_open},
  {"read", unix_read},
  {"write", unix_write},
  {"lseek", unix_lseek},
  {"stat", unix_stat},
  {"fstat", unix_fstat},
  {"getpid", unix_getpid},
  {"getppid", unix_getppid},
  {"setpgid", unix_setpgid},
  {"getpgid", unix_getpgid},
  {"unlink", unix_unlink},
  {"rmdir", unix_rmdir},
  {"access", unix_access},
  {"kill", unix_kill},
  {"killpg", unix_killpg},
  {NULL, NULL}};

#define V(x) {#x, x}
static const named_int_t tab_ints[] = {
  /* open(2) flags */
  V(O_RDONLY), V(O_WRONLY), V(O_RDWR),
  /* access(2) mode */
  V(R_OK), V(W_OK), V(X_OK),
  /* signal numbers */
  V(SIGINT), V(SIGILL), V(SIGABRT), V(SIGFPE), V(SIGSEGV), V(SIGKILL),
  V(SIGTERM), V(SIGCHLD), V(SIGUSR1), V(SIGUSR2), V(SIGBUS),
  /* dirent.d_type flags */
  V(DT_FIFO), V(DT_CHR), V(DT_DIR), V(DT_BLK), V(DT_REG), V(DT_LNK), V(DT_SOCK),
  V(DT_WHT),
  /* lseek(2) flags */
  V(SEEK_SET), V(SEEK_CUR), V(SEEK_END),
  /* terminator */
  {NULL, 0}};
#undef V
/* clang-format on */

LUAMOD_API int luaopen_unix(lua_State *L) {
  luaL_newlib(L, tab_funcs);
  for (const named_int_t *ni = tab_ints; ni->name; ni++) {
    lua_pushinteger(L, ni->value);
    lua_setfield(L, -2, ni->name);
  }
  return 1;
}
