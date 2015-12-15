/*******************************************************************************
 *
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/******************************************************************************
*                 file : $RCSfile: strsignal.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
FUNCTION
	<<strsignal>>---convert signal number to string

INDEX
	strsignal

ANSI_SYNOPSIS
	#include <string.h>
	char *strsignal(int <[signal]>);

TRAD_SYNOPSIS
	#include <string.h>
	char *strsignal(<[signal]>)
	int <[signal]>;

DESCRIPTION
<<strsignal>> converts the signal number <[signal]> into a
string.  If <[signal]> is not a known signal number, the result
will be of the form "Unknown signal NN" where NN is the <[signal]>
is a decimal number.

RETURNS
This function returns a pointer to a string.  Your application must
not modify that string.

PORTABILITY
POSIX.1-2008 C requires <<strsignal>>, but does not specify the strings used
for each signal number.

<<strsignal>> requires no supporting OS subroutines.

QUICKREF
	strsignal pure
*/

/*
 *  Written by Joel Sherrill <joel.sherrill@OARcorp.com>.
 *
 *  COPYRIGHT (c) 2010.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  Permission to use, copy, modify, and distribute this software for any
 *  purpose without fee is hereby granted, provided that this entire notice
 *  is included in all copies of any software which is or includes a copy
 *  or modification of this software.
 *
 *  THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 *  WARRANTY.  IN PARTICULAR,  THE AUTHOR MAKES NO REPRESENTATION
 *  OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
 *  SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 *  $Id: //mips/sw/code/emlibs/MAIN/mips/emlibs/smallclib/string/strsignal.c#2 $
 */

/* same as newlib 2.0 */

#include <string.h>
#include <signal.h>
#include <stdio.h>

static char buf[40];

char *
_DEFUN (strsignal, (signal),
	int signal)
{
  char *buffer;

  buffer = buf;

#if defined(SIGRTMIN) && defined(SIGRTMAX)
  if ((signal >= SIGRTMIN) || (signal <= SIGRTMAX)) {
    siprintf (buffer, "Real-time signal %d", signal - SIGRTMIN);
    return buffer;
  }
#endif

  switch (signal) {
#ifdef SIGHUP
    case SIGHUP:
      buffer = "Hangup";
      break;
#endif
#ifdef SIGINT
    case SIGINT:
      buffer = "Interrupt";
      break;
#endif
#ifdef SIGQUIT
    case SIGQUIT:
      buffer = "Quit";
      break;
#endif
#ifdef SIGILL
    case SIGILL:
      buffer = "Illegal instruction";
      break;
#endif
#ifdef SIGTRAP
    case SIGTRAP:
      buffer = "Trace/breakpoint trap";
      break;
#endif
#ifdef SIGIOT
  #if  defined(SIGABRT) && (SIGIOT != SIGABRT)
    case SIGABRT:
  #endif
    case SIGIOT:
      buffer = "IOT trap";
      break;
#endif
#ifdef SIGEMT
    case SIGEMT:
      buffer = "EMT trap";
      break;
#endif
#ifdef SIGFPE
    case SIGFPE:
      buffer = "Floating point exception";
      break;
#endif
#ifdef SIGKILL
    case SIGKILL:
      buffer = "Killed";
      break;
#endif
#ifdef SIGBUS
    case SIGBUS:
      buffer = "Bus error";
      break;
#endif
#ifdef SIGSEGV
    case SIGSEGV:
      buffer = "Segmentation fault";
      break;
#endif
#ifdef SIGSYS
    case SIGSYS:
      buffer = "Bad system call";
      break;
#endif
#ifdef SIGPIPE
    case SIGPIPE:
      buffer = "Broken pipe";
      break;
#endif
#ifdef SIGALRM
    case SIGALRM:
      buffer = "Alarm clock";
      break;
#endif
#ifdef SIGTERM
    case SIGTERM:
      buffer = "Terminated";
      break;
#endif
#ifdef SIGURG
    case SIGURG:
      buffer = "Urgent I/O condition";
      break;
#endif
#ifdef SIGSTOP
    case SIGSTOP:
      buffer = "Stopped (signal)";
      break;
#endif
#ifdef SIGTSTP
    case SIGTSTP:
      buffer = "Stopped";
      break;
#endif
#ifdef SIGCONT
    case SIGCONT:
      buffer = "Continued";
      break;
#endif
#ifdef SIGCHLD
  #if  defined(SIGCLD) && (SIGCHLD != SIGCLD)
    case SIGCLD:
  #endif
    case SIGCHLD:
      buffer = "Child exited";
      break;
#endif
#ifdef SIGTTIN
    case SIGTTIN:
      buffer = "Stopped (tty input)";
      break;
#endif
#ifdef SIGTTOUT
    case SIGTTOUT:
      buffer = "Stopped (tty output)";
      break;
#endif
#ifdef SIGIO
  #if  defined(SIGPOLL) && (SIGIO != SIGPOLL)
    case SIGPOLL:
  #endif
    case SIGIO:
      buffer = "I/O possible";
      break;
#endif
#ifdef SIGWINCH
    case SIGWINCH:
      buffer = "Window changed";
      break;
#endif
#ifdef SIGUSR1
    case SIGUSR1:
      buffer = "User defined signal 1";
      break;
#endif
#ifdef SIGUSR2
    case SIGUSR2:
      buffer = "User defined signal 2";
      break;
#endif
#ifdef SIGPWR
    case SIGPWR:
      buffer = "Power Failure";
      break;
#endif
#ifdef SIGXCPU
    case SIGXCPU:
      buffer = "CPU time limit exceeded";
      break;
#endif
#ifdef SIGXFSZ
    case SIGXFSZ:
      buffer = "File size limit exceeded";
      break;
#endif
#ifdef SIGVTALRM 
    case SIGVTALRM :
      buffer = "Virtual timer expired";
      break;
#endif
#ifdef SIGPROF
    case SIGPROF:
      buffer = "Profiling timer expired";
      break;
#endif
#if defined(SIGLOST) && SIGLOST != SIGPWR
    case SIGLOST:
      buffer = "Resource lost";
      break;
#endif
    default:
      siprintf (buffer, "Unknown signal %d", signal);
      break;
  }

  return buffer;
}
