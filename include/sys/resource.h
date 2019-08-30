/*	$NetBSD: resource.h,v 1.36 2019/03/30 23:29:55 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
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
 *
 *	@(#)resource.h	8.4 (Berkeley) 1/9/95
 */

#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

#include <sys/time.h>

/*
 * Process priority specifications to get/setpriority.
 */
#define PRIO_MIN -20
#define PRIO_MAX 20

#define PRIO_PROCESS 0
#define PRIO_PGRP 1
#define PRIO_USER 2

/*
 * Resource utilization information.
 */

#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN -1

struct rusage {
  struct timeval ru_utime; /* user time used */
  struct timeval ru_stime; /* system time used */
  long ru_maxrss;          /* max resident set size */
#define ru_first ru_ixrss
  long ru_ixrss;    /* integral shared memory size */
  long ru_idrss;    /* integral unshared data " */
  long ru_isrss;    /* integral unshared stack " */
  long ru_minflt;   /* page reclaims */
  long ru_majflt;   /* page faults */
  long ru_nswap;    /* swaps */
  long ru_inblock;  /* block input operations */
  long ru_oublock;  /* block output operations */
  long ru_msgsnd;   /* messages sent */
  long ru_msgrcv;   /* messages received */
  long ru_nsignals; /* signals received */
  long ru_nvcsw;    /* voluntary context switches */
  long ru_nivcsw;   /* involuntary " */
#define ru_last ru_nivcsw
};

#ifndef _KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
int getrusage(int, struct rusage *);
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_SYS_RESOURCE_H_ */
