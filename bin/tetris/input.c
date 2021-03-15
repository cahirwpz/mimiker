/*	$NetBSD: input.c,v 1.11 2009/05/25 04:33:53 dholland Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek and Darren F. Provine.
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
 *
 *	@(#)input.c	8.1 (Berkeley) 5/31/93
 */

/*
 * Tetris input.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#include <errno.h>
#include <unistd.h>

#include "input.h"
#include "tetris.h"

/* return true iff the given timeval is positive */
#define TV_POS(tv)                                                             \
  ((tv)->tv_sec > 0 || ((tv)->tv_sec == 0 && (tv)->tv_usec > 0))

/* subtract timeval `sub' from `res' */
#define TV_SUB(res, sub)                                                       \
  (res)->tv_sec -= (sub)->tv_sec;                                              \
  (res)->tv_usec -= (sub)->tv_usec;                                            \
  if ((res)->tv_usec < 0) {                                                    \
    (res)->tv_usec += 1000000;                                                 \
    (res)->tv_sec--;                                                           \
  }

static int have_char;
static char buffered_char;

static sig_atomic_t sigalrm_handled = 0;

static void handle_sigalrm(int signo) {
  sigalrm_handled = 1;
}

void input_init(void) {
  signal(SIGALRM, handle_sigalrm);
}

static void read_one(char *c) {
  if (!have_char)
    stop("read_one(): no buffered char");
  have_char = 0;
  *c = buffered_char;
}

/*
 * Do a `read wait': poll for reading from stdin, with timeout *tvp.
 * On return, modify *tvp to reflect the amount of time spent waiting.
 * It will be positive only if input appeared before the time ran out;
 * otherwise it will be zero or perhaps negative.
 *
 * If tvp is nil, wait forever, but return if poll is interrupted.
 *
 * Return 0 => no input, 1 => can read_one()
 */
int rwait(struct timeval *tvp) {

  if (have_char)
    return 1;

  struct itimerval it;
  if (tvp && TV_POS(tvp)) {
    it.it_value = *tvp;
    it.it_interval = (struct timeval){0};
    setitimer(ITIMER_REAL, &it, NULL);
  }

again:
  /* XXX: there is a race here: we only want to read() if
   * we haven't yet received SIGALRM, but we can always get a SIGALRM
   * between the check and the read(). We just hope that it doesn't happen. */
  if (sigalrm_handled)
    goto timeout;
  switch (read(STDIN_FILENO, &buffered_char, 1)) {
    case -1:
      if (errno == EINTR) {
        if (sigalrm_handled) {
          /* SIGALRM: timed out */
        timeout:
          sigalrm_handled = 0;
          if (tvp) {
            tvp->tv_sec = 0;
            tvp->tv_usec = 0;
          }
          return (0);
        }
        /* other signal: try again. */
        goto again;
      }
      stop("read failed");
    case 0:
      stop("empty read");
  }

  /* got input */
  have_char = 1;
  static struct itimerval zero_it;

  if (tvp && TV_POS(tvp)) {
    /* since there is input, we may not have timed out */
    setitimer(ITIMER_REAL, &zero_it, &it);
    *tvp = it.it_value;
    sigalrm_handled = 0;
  }
  return (1);
}

/*
 * `sleep' for the current turn time.
 * Eat any input that might be available.
 */
void tsleep(void) {
  struct timeval tv;
  char c;

  tv.tv_sec = 0;
  tv.tv_usec = fallrate;
  while (TV_POS(&tv))
    if (rwait(&tv)) {
      read_one(&c);
      break;
    }
}

/*
 * getchar with timeout.
 */
int tgetchar(void) {
  static struct timeval timeleft;
  char c;

  /*
   * Reset timeleft to fallrate whenever it is not positive.
   * In any case, wait to see if there is any input.  If so,
   * take it, and update timeleft so that the next call to
   * tgetchar() will not wait as long.  If there is no input,
   * make timeleft zero or negative, and return -1.
   *
   * Most of the hard work is done by rwait().
   */
  if (!TV_POS(&timeleft)) {
    faster(); /* go faster */
    timeleft.tv_sec = 0;
    timeleft.tv_usec = fallrate;
  }
  if (!rwait(&timeleft))
    return (-1);
  read_one(&c);
  return ((int)(unsigned char)c);
}
