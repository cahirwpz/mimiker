/*	$NetBSD: ttycom.h,v 1.21 2017/10/25 06:32:59 kre Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ttycom.h	8.1 (Berkeley) 3/28/94
 */

#ifndef _SYS_TTYCOM_H_
#define _SYS_TTYCOM_H_

#include <sys/syslimits.h>
#include <sys/ioccom.h>

/*
 * Tty ioctl's except for those supported only for backwards compatibility
 * with the old tty driver.
 */

/*
 * Window/terminal size structure.  This information is stored by the kernel
 * in order to provide a consistent interface, but is not used by the kernel.
 */
struct winsize {
  unsigned short ws_row;    /* rows, in characters */
  unsigned short ws_col;    /* columns, in characters */
  unsigned short ws_xpixel; /* horizontal size, pixels */
  unsigned short ws_ypixel; /* vertical size, pixels */
};

/*
 * This is the maximum length of a line discipline's name.
 */
#define TTLINEDNAMELEN 32
typedef char linedn_t[TTLINEDNAMELEN];

#define TIOCGLINED _IOR('t', 66, linedn_t)      /* get line discipline (new) */
#define TIOCGETA _IOR('t', 19, struct termios)  /* get termios struct */
#define TIOCSETA _IOW('t', 20, struct termios)  /* set termios struct */
#define TIOCSETAW _IOW('t', 21, struct termios) /* drain output, set */
#define TIOCSETAF _IOW('t', 22, struct termios) /* drn out, fls in, set */
#define TIOCSCTTY _IO('t', 97)                  /* become controlling tty */
#define TIOCGWINSZ _IOR('t', 104, struct winsize) /* get window size */
#define TIOCSWINSZ _IOW('t', 103, struct winsize) /* set window size */
#define TIOCPTSNAME _IOW('t', 105, char *)        /* get slave device path */
#define TIOCSTART _IO('t', 110)                   /* start output, like ^Q */
#define TIOCSTOP _IO('t', 111)                    /* stop output, like ^S */
#define TIOCGPGRP _IOR('t', 119, int)             /* get pgrp of tty */
#define TIOCSPGRP _IOW('t', 118, int)             /* set pgrp of tty */
#define TIOCGQSIZE _IOR('t', 129, int)            /* get queue size */

#endif /* !_SYS_TTYCOM_H_ */
