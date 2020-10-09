/*  $NetBSD: su.c,v 1.72 2015/06/16 22:54:11 christos Exp $ */

/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *  may be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <util.h>

#ifdef ALLOW_GROUP_CHANGE
#include "grutil.h"
#endif
#include "suutil.h"

#define ARGSTR "-dflm"

#ifndef SU_GROUP
#define SU_GROUP  "wheel"
#endif

#define GROUP_PASSWORD  "Group Password:"

static int check_ingroup(int, const char *, const char *, int);

int
main(int argc, char **argv)
{
  extern char **environ;
  struct passwd *pwd;
  char *p;
  uid_t ruid;
  int asme, ch, asthem, fastlogin, gohome;
/* TODO(fzdob): inactive unless we had get/setpriority functions 
 *       (only there we use prio variable) */
#if 0
  int prio;
#endif
  enum { UNSET, YES, NO } iscsh = UNSET;
  const char *user, *shell, *avshell;
  char *username, **np;
#ifdef SU_ROOTAUTH
  char *userpass;
#endif
  char *class;
  char shellbuf[MAXPATHLEN], avshellbuf[MAXPATHLEN];
#ifdef ALLOW_GROUP_CHANGE
  char *gname;
#endif

  (void)setprogname(argv[0]);
  asme = asthem = fastlogin = 0;
  gohome = 1;
  shell = class = NULL;
  while ((ch = getopt(argc, argv, ARGSTR)) != -1)
    switch((char)ch) {
    case 'd':
      asme = 0;
      asthem = 1;
      gohome = 0;
      break;
    case 'f':
      fastlogin = 1;
      break;
    case '-':
    case 'l':
      asme = 0;
      asthem = 1;
      break;
    case 'm':
      asme = 1;
      asthem = 0;
      break;
    case '?':
    default:
      (void)fprintf(stderr,
#ifdef ALLOW_GROUP_CHANGE
        "usage: %s [%s] [login[:group] [shell arguments]]\n",
#else
        "usage: %s [%s] [login [shell arguments]]\n",
#endif
        getprogname(), ARGSTR);
      exit(EXIT_FAILURE);
    }

  argv += optind;

/* TODO(fzdob): inactive unless we had get/setpriority functions */
#if 0
  /* Lower the priority so su runs faster */
  errno = 0;
  prio = getpriority(PRIO_PROCESS, 0);
  if (errno)
    prio = 0;
  if (prio > -2)
    (void)setpriority(PRIO_PROCESS, 0, -2);
#endif

  /* get current login name and shell */
  ruid = getuid();
  username = getlogin();
  if (username == NULL || (pwd = getpwnam(username)) == NULL ||
    pwd->pw_uid != ruid)
    pwd = getpwuid(ruid);
  if (pwd == NULL)
    errx(EXIT_FAILURE, "who are you?");
  username = estrdup(pwd->pw_name);
#ifdef SU_ROOTAUTH
  userpass = estrdup(pwd->pw_passwd);
#endif

  if (asme) {
    if (pwd->pw_shell && *pwd->pw_shell) {
      (void)estrlcpy(shellbuf, pwd->pw_shell, sizeof(shellbuf));
      shell = shellbuf;
    } else {
      shell = _PATH_BSHELL;
      iscsh = NO;
    }
  }
  /* get target login information, default to root */
  user = *argv ? *argv : "root";
  np = *argv ? argv : argv - 1;

#ifdef ALLOW_GROUP_CHANGE
  if ((p = strchr(user, ':')) != NULL) {
    *p = '\0';
    gname = ++p;
  }
  else
    gname = NULL;

#ifdef ALLOW_EMPTY_USER
  if (user[0] == '\0' && gname != NULL)
    user = username;
#endif
#endif
  if ((pwd = getpwnam(user)) == NULL)
    errx(EXIT_FAILURE, "unknown login `%s'", user);

  if (ruid) {
/* TODO(fzdob): inactive unless we had crytp and getpass functions */
#if 0
    char *pass = strdup(pwd->pw_passwd);
#endif
    int ok = pwd->pw_uid != 0;

#ifdef SU_ROOTAUTH
    /*
     * Allow those in group rootauth to su to root, by supplying
     * their own password.
     */
    if (!ok) {
      if ((ok = check_ingroup(-1, SU_ROOTAUTH, username, 0))) {
/* TODO(fzdob): inactive unless we had crytp and getpass functions */
#if 0
        pass = userpass;
#endif
        user = username;
      }
    }
#endif
    /*
     * Only allow those in group SU_GROUP to su to root,
     * but only if that group has any members.
     * If SU_GROUP has no members, allow anyone to su root
     */
    if (!ok)
      ok = check_ingroup(-1, SU_GROUP, username, 1);
    if (!ok)
      errx(EXIT_FAILURE,
    "you are not listed in the correct secondary group (%s) to su %s.",
            SU_GROUP, user);

/* TODO(fzdob): inactive unless we had crytp and getpass functions */
#if 0
    /* if target requires a password, verify it */
    if (*pass && pwd->pw_uid != ruid) { /* XXX - OK? */
      p = getpass("Password:");
      if (strcmp(pass, crypt(p, pass)) != 0) {
        (void)fprintf(stderr, "Sorry\n");
        syslog(LOG_WARNING,
          "BAD SU %s to %s%s", username,
          pwd->pw_name, ontty());
        exit(EXIT_FAILURE);
      }
    }
#endif
  }

  if (asme) {
    /* if asme and non-standard target shell, must be root */
    if (chshell(pwd->pw_shell) == 0 && ruid)
      errx(EXIT_FAILURE, "permission denied (shell).");
  } else if (pwd->pw_shell && *pwd->pw_shell) {
    shell = pwd->pw_shell;
    iscsh = UNSET;
  } else {
    shell = _PATH_BSHELL;
    iscsh = NO;
  }

  if ((p = strrchr(shell, '/')) != NULL)
    avshell = p+1;
  else
    avshell = shell;

  /* if we're forking a csh, we want to slightly muck the args */
  if (iscsh == UNSET)
    iscsh = strstr(avshell, "csh") ? YES : NO;

  /* set permissions */
  if (setgid(pwd->pw_gid) == -1)
    err(EXIT_FAILURE, "setgid");
  /* if we aren't changing users, keep the current group members */
  if (ruid != pwd->pw_uid && initgroups(user, pwd->pw_gid) != 0)
    errx(EXIT_FAILURE, "initgroups failed");
#ifdef ALLOW_GROUP_CHANGE
  addgroup(/*EMPTY*/, gname, pwd, ruid, GROUP_PASSWORD);
#endif
  if (setuid(pwd->pw_uid) == -1)
    err(EXIT_FAILURE, "setuid");

  if (!asme) {
    if (asthem) {
      p = getenv("TERM");
      /* Create an empty environment */
      environ = emalloc(sizeof(char *));
      environ[0] = NULL;
      (void)setenv("PATH", _PATH_DEFPATH, 1);
      if (p)
        (void)setenv("TERM", p, 1);
      if (gohome && chdir(pwd->pw_dir) == -1)
        errx(EXIT_FAILURE, "no directory");
    }

    if (asthem || pwd->pw_uid) {
      (void)setenv("LOGNAME", pwd->pw_name, 1);
      (void)setenv("USER", pwd->pw_name, 1);
    }
    (void)setenv("HOME", pwd->pw_dir, 1);
    (void)setenv("SHELL", shell, 1);
  }
  (void)setenv("SU_FROM", username, 1);

  if (iscsh == YES) {
    if (fastlogin)
      *np-- = __UNCONST("-f");
    if (asme)
      *np-- = __UNCONST("-m");
  } else {
    if (fastlogin)
      (void)unsetenv("ENV");
  }

  if (asthem) {
    avshellbuf[0] = '-';
    (void)estrlcpy(avshellbuf + 1, avshell, sizeof(avshellbuf) - 1);
    avshell = avshellbuf;
  } else if (iscsh == YES) {
    /* csh strips the first character... */
    avshellbuf[0] = '_';
    (void)estrlcpy(avshellbuf + 1, avshell, sizeof(avshellbuf) - 1);
    avshell = avshellbuf;
  }
  *np = __UNCONST(avshell);

  if (ruid != 0)
    syslog(LOG_NOTICE, "%s to %s%s",
      username, pwd->pw_name, ontty());

/* TODO(fzdob): inactive unless we had get/setpriority functions */
#if 0
  /* Raise our priority back to what we had before */
  (void)setpriority(PRIO_PROCESS, 0, prio);
#endif

  (void)execv(shell, np);
  err(EXIT_FAILURE, "%s", shell);
  /* NOTREACHED */
}

static int
check_ingroup(int gid, const char *gname, const char *user, int ifempty)
{
  struct group *gr;
  const char * const*g;
#ifdef SU_INDIRECT_GROUP
  char **gr_mem;
  int n = 0;
  int i = 0;
#endif
  int ok = 0;

  if (gname == NULL)
    gr = getgrgid((gid_t) gid);
  else
    gr = getgrnam(gname);

  /*
   * XXX we are relying on the fact that we only set ifempty when
   * calling to check for SU_GROUP and that is the only time a
   * missing group is acceptable.
   */
  if (gr == NULL)
    return ifempty;
  if (!*gr->gr_mem)     /* empty */
    return ifempty;

  /*
   * Ok, first see if user is in gr_mem
   */
  for (g = gr->gr_mem; *g; ++g) {
    if (strcmp(*g, user) == 0)
      return 1;   /* ok */
#ifdef SU_INDIRECT_GROUP
    ++n;      /* count them */
#endif
  }
#ifdef SU_INDIRECT_GROUP
  /*
   * No.
   * Now we need to duplicate the gr_mem list, and recurse for
   * each member to see if it is a group, and if so whether user is
   * in it.
   */
  gr_mem = emalloc((n + 1) * sizeof (char *));
  for (g = gr->gr_mem, i = 0; *g; ++g) {
    gr_mem[i] = estrdup(*g);
    i++;
  }
  gr_mem[i++] = NULL;

  for (g = gr_mem; ok == 0 && *g; ++g) {
    /*
     * If we get this far we don't accept empty/missing groups.
     */
    ok = check_ingroup(-1, *g, user, 0);
  }
  for (g = gr_mem; *g; ++g) {
    free(*g);
  }
  free(gr_mem);
#endif
  return ok;
}

