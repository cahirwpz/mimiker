/*	$NetBSD: pty.c,v 1.4 2014/01/08 02:17:30 christos Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: pty.c,v 1.4 2014/01/08 02:17:30 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ttycom.h>
#include <sys/ioctl.h>

/*
 * XXX: This is a stub.
 * This function is meant to set the ownership of the slave device to the
 * calling process. FreeBSD sets the slave device ownership at creation time, so
 * their grantpt() is essentialy a no-op.
 * TODO: Set slave device ownership at creation time (FreeBSD-style)
 * or here, using an ioctl() (NetBSD-style)
 */
int grantpt(int fildes) {
  return 0;
}

/*
 * This is NOT a stub. The slave device is available from the moment it is
 * created, so unlockpt() is a no-op in our case.
 */
int unlockpt(int fildes) {
  return 0;
}

char *ptsname(int fildes) {
  static char buf[PATH_MAX];
  char *bufptr = buf;

  if (ioctl(fildes, TIOCPTSNAME, &bufptr) == -1)
    return NULL;

  return bufptr;
}

int ptsname_r(int fildes, char *buf, size_t buflen) {
  char p[PATH_MAX];
  char *bufptr = p;

  if (buf == NULL)
    return EINVAL;
  if (ioctl(fildes, TIOCPTSNAME, &bufptr) == -1)
    return errno;
  if (strlcpy(buf, p, buflen) > buflen)
    return ERANGE;
  return 0;
}
