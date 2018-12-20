/* Adapted from:
 * https://github.com/JakubSzczerbinski/luash/blob/master/syscalls.c */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "lua.h"
#include "lauxlib.h"

static int unix_fork(lua_State *L) {
  pid_t result = fork();
  lua_pushnumber(L, result);
  return 1;
}

static int unix_execve(lua_State *L) {
  int result;
  int i = 0;

  const char *path = luaL_checkstring(L, 1);
  char **argv = (char **)malloc(sizeof(char *) * ARG_MAX);

  if (!lua_istable(L, 2)) {
    result = -1;
    goto end;
  }

  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (i >= ARG_MAX) {
      result = -1;
      goto end;
    }
    if (!lua_isstring(L, -1)) {
      result = -1;
      goto end;
    }

    const char *str = lua_tostring(L, -1);
    argv[i] = (char *)malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(argv[i], str);

    lua_pop(L, 1);
    i++;
  }
  argv[i] = NULL;

  result = execve(path, argv, environ);
  if (result != 0)
    result = errno;
end:
  lua_pushnumber(L, result);
  free(argv);
  return 1;
}

static int lua_waitpid(lua_State *L) {
  pid_t pid = luaL_checkinteger(L, 1);
  int wstatus;
  int options = luaL_checkinteger(L, 2);
  int result = waitpid(pid, &wstatus, options);
  lua_pushnumber(L, result);
  lua_pushnumber(L, wstatus);
  return 2;
}

static int lua_pipe(lua_State *L) {
  int fd[2];
  int result = pipe(fd);
  lua_pushnumber(L, fd[0]);
  lua_pushnumber(L, fd[1]);
  lua_pushnumber(L, result);
  return 3;
}

static int unix_close(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int result = close(fd);
  lua_pushnumber(L, result);
  return 1;
}

static int unix_dup2(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int fd2 = luaL_checkinteger(L, 2);
  int result = dup2(fd, fd2);
  lua_pushnumber(L, result);
  return 1;
}

static int unix_exit(lua_State *L) {
  int status = luaL_checkinteger(L, 1);
  exit(status);
  return 0;
}

static int unix_getcwd(lua_State *L) {
  char *buff = (char *)malloc(sizeof(char) * 4096);
  getcwd(buff, 4096);
  lua_pushstring(L, buff);
  free(buff);
  return 1;
}

static int unix_chdir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int result = chdir(path);
  lua_pushinteger(L, result);
  return 1;
}

static int unix_getdirentries(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  char *buf = (char *)malloc(sizeof(char) * 4096);
  off_t base_p = 0;
  size_t size_read = 0;
  struct dirent *dir;
  lua_createtable(L, 0, 0);
  int i = 1;
  do {
    size_read = getdirentries(fd, buf, 4096, &base_p);
    dir = (struct dirent *)buf;
    while ((char *)dir < buf + size_read) {
      lua_pushinteger(L, i++);
      lua_createtable(L, 0, 4);

      lua_pushstring(L, "d_type");
      lua_pushinteger(L, dir->d_type);
      lua_settable(L, -3);

      lua_pushstring(L, "d_fileno");
      lua_pushinteger(L, dir->d_fileno);
      lua_settable(L, -3);

      lua_pushstring(L, "d_name");
      lua_pushstring(L, dir->d_name);
      lua_settable(L, -3);

      lua_settable(L, -3);
      dir = (struct dirent *)((char *)dir + dir->d_reclen);
    }
  } while (size_read > 0);
  return 1;
}

static int unix_open(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int fd = open(path, 0);
  lua_pushinteger(L, fd);
  return 1;
}

static const luaL_Reg tab_funcs[] = {
  {"getcwd", unix_getcwd},  {"chdir", unix_chdir},
  {"execve", unix_execve},  {"fork", unix_fork},
  {"waitpid", lua_waitpid}, {"pipe", lua_pipe},
  {"close", unix_close},    {"dup2", unix_dup2},
  {"exit", unix_exit},      {"getdirentries", unix_getdirentries},
  {"open", unix_open},      {NULL, NULL}};

LUAMOD_API int luaopen_unix(lua_State *L) {
  luaL_newlib(L, tab_funcs);
  return 1;
}
