/*	$NetBSD: statvfs.h,v 1.18 2013/04/05 17:34:27 christos Exp $	 */

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

#ifndef _SYS_STATVFS_H_
#define _SYS_STATVFS_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#define MFSNAMELEN 32
#define MNAMELEN 1024

struct statvfs {
  unsigned long f_bsize;  /* file system block size */
  unsigned long f_frsize; /* fundamental file system block size */

  /* The following are in units of f_frsize */
  fsblkcnt_t f_blocks; /* number of blocks in file system, */
  fsblkcnt_t f_bfree;  /* free blocks avail in file system */
  fsblkcnt_t f_bavail; /* free blocks avail to non-root */

  fsfilcnt_t f_files;  /* total file nodes in file system */
  fsfilcnt_t f_ffree;  /* free file nodes in file system */
  fsfilcnt_t f_favail; /* free file nodes avail to non-root */

  unsigned long f_fsid;    /* Posix compatible fsid */
  unsigned long f_flag;    /* bit mask of f_flag values */
  unsigned long f_namemax; /* maximum filename length */

  char f_fstypename[MFSNAMELEN]; /* fs type name */
  char f_mntonname[MNAMELEN];    /* directory on which mounted */
  char f_mntfromname[MNAMELEN];  /* mounted file system */
};

#ifndef _KERNEL
__BEGIN_DECLS
int getmntinfo(struct statvfs **, int);

int statvfs(const char *__restrict, struct statvfs *__restrict);
int fstatvfs(int, struct statvfs *);
int getvfsstat(struct statvfs *, size_t, int);
__END_DECLS
#endif

#endif /* !_SYS_STATVFS_H_ */
