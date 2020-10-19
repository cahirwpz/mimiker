/*	$NetBSD: syslog.h,v 1.41 2017/03/22 17:52:36 roy Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)syslog.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _SYS_SYSLOG_H_
#define _SYS_SYSLOG_H_

#include <sys/cdefs.h>

#define _PATH_LOG "/var/run/log"

/*
 * priorities/facilities are encoded into a single 32-bit quantity, where the
 * bottom 3 bits are the priority (0-7) and the top 28 bits are the facility
 * (0-big number).  Both the priorities and the facilities map roughly
 * one-to-one to strings in the syslogd(8) source code.  This mapping is
 * included in this file.
 *
 * priorities (these are ordered)
 */
#define LOG_EMERG 0   /* system is unusable */
#define LOG_ALERT 1   /* action must be taken immediately */
#define LOG_CRIT 2    /* critical conditions */
#define LOG_ERR 3     /* error conditions */
#define LOG_WARNING 4 /* warning conditions */
#define LOG_NOTICE 5  /* normal but significant condition */
#define LOG_INFO 6    /* informational */
#define LOG_DEBUG 7   /* debug-level messages */

#define LOG_PRIMASK 0x07 /* mask to extract priority part (internal) */
                         /* extract priority */
#define LOG_PRI(p) ((p)&LOG_PRIMASK)
#define LOG_MAKEPRI(fac, pri) (((fac) << 3) | (pri))

/* facility codes */
#define LOG_KERN (0 << 3)      /* kernel messages */
#define LOG_USER (1 << 3)      /* random user-level messages */
#define LOG_MAIL (2 << 3)      /* mail system */
#define LOG_DAEMON (3 << 3)    /* system daemons */
#define LOG_AUTH (4 << 3)      /* security/authorization messages */
#define LOG_SYSLOG (5 << 3)    /* messages generated internally by syslogd */
#define LOG_LPR (6 << 3)       /* line printer subsystem */
#define LOG_NEWS (7 << 3)      /* network news subsystem */
#define LOG_UUCP (8 << 3)      /* UUCP subsystem */
#define LOG_CRON (9 << 3)      /* clock daemon */
#define LOG_AUTHPRIV (10 << 3) /* security/authorization messages (private) */
#define LOG_FTP (11 << 3)      /* ftp daemon */

#ifndef _KERNEL

/* Used by reentrant functions */

struct syslog_data {
  int log_version;
  int log_file;
  int log_connected;
  int log_opened;
  int log_stat;
  const char *log_tag;
  char log_hostname[256]; /* MAXHOSTNAMELEN */
  int log_fac;
  int log_mask;
};

void syslog(int, const char *, ...) __sysloglike(2, 3);
void vsyslog(int, const char *, __va_list) __sysloglike(2, 0);

#endif /* _KERNEL */

#endif /* !_SYS_SYSLOG_H_ */
