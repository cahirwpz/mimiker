/*	$NetBSD: siglist.c,v 1.18 2016/07/09 21:15:00 dholland Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#include <signal.h>

static const char *const __siglist[] = {
  "Signal 0",                 /* 0 */
  "Hangup",                   /* 1 SIGHUP */
  "Interrupt",                /* 2 SIGINT */
  "Quit",                     /* 3 SIGQUIT */
  "Illegal instruction",      /* 4 SIGILL */
  "Trace/BPT trap",           /* 5 SIGTRAP */
  "Abort trap",               /* 6 SIGABRT */
  "EMT trap",                 /* 7 SIGEMT */
  "Floating point exception", /* 8 SIGFPE */
  "Killed",                   /* 9 SIGKILL */
  "Bus error",                /* 10 SIGBUS */
  "Segmentation fault",       /* 11 SIGSEGV */
  "Bad system call",          /* 12 SIGSYS */
  "Broken pipe",              /* 13 SIGPIPE */
  "Alarm clock",              /* 14 SIGALRM */
  "Terminated",               /* 15 SIGTERM */
  "Urgent I/O condition",     /* 16 SIGURG */
  "Suspended (signal)",       /* 17 SIGSTOP */
  "Suspended",                /* 18 SIGTSTP */
  "Continued",                /* 19 SIGCONT */
  "Child exited",             /* 20 SIGCHLD */
  "Stopped (tty input)",      /* 21 SIGTTIN */
  "Stopped (tty output)",     /* 22 SIGTTOU */
  "I/O possible",             /* 23 SIGIO */
  "CPU time limit exceeded",  /* 24 SIGXCPU */
  "File size limit exceeded", /* 25 SIGXFSZ */
  "Virtual timer expired",    /* 26 SIGVTALRM */
  "Profiling timer expired",  /* 27 SIGPROF */
  "Window size changed",      /* 28 SIGWINCH */
  "Information request",      /* 29 SIGINFO */
  "User defined signal 1",    /* 30 SIGUSR1 */
  "User defined signal 2",    /* 31 SIGUSR2 */
  "Power fail/restart",       /* 32 SIGPWR */
};

const int sys_nsig = sizeof(__siglist) / sizeof(__siglist[0]);

const char *const *sys_siglist = __siglist;
