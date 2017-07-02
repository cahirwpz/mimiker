/*-
 * Copyright (c) 1992 Keith Muller.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego.
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
 *	@(#)cpio.h	8.1 (Berkeley) 5/31/93
 */

#ifndef _SYS_CPIO_H_
#define _SYS_CPIO_H_

/*
 * Defines common to all versions of cpio
 */
#define CPIO_TRAILER "TRAILER!!!" /* name in last archive record */

/*
 * Header encoding of the different file types
 */
typedef enum {
  C_FIFO = 1, /* FIFO */
  C_CHR = 2,  /* Character special file */
  C_DIR = 4,  /* Directory */
  C_BLK = 6,  /* Block special file */
  C_REG = 8,  /* Regular file */
  C_CTG = 9,  /* Reserved for contiguous files */
  C_LNK = 10, /* Reserved for symbolic links */
  C_SOCK = 12 /* Reserved for sockets */
} cpio_filetype_t;

/* cpio mode to file type */
static inline cpio_filetype_t CMTOFT(unsigned mode) {
  return ((mode)&0170000) >> 12;
}

/*
 * Data Interchange Format - Extended cpio header format - POSIX 1003.1-1990
 */
typedef struct {
  char c_magic[6];     /* magic cookie */
  char c_dev[6];       /* device number */
  char c_ino[6];       /* inode number */
  char c_mode[6];      /* file type/access */
  char c_uid[6];       /* owners uid */
  char c_gid[6];       /* owners gid */
  char c_nlink[6];     /* # of links at archive creation */
  char c_rdev[6];      /* block/char major/minor # */
  char c_mtime[11];    /* modification time */
  char c_namesize[6];  /* length of pathname */
  char c_filesize[11]; /* length of file in bytes */
} cpio_old_hdr_t;

#define CPIO_OMAGIC 070707 /* transportable archive id */

/*
 * SVR4 cpio header structure (with/without file data crc)
 */
typedef struct {
  char c_magic[6];    /* magic cookie */
  char c_ino[8];      /* inode number */
  char c_mode[8];     /* file type/access */
  char c_uid[8];      /* owners uid */
  char c_gid[8];      /* owners gid */
  char c_nlink[8];    /* # of links at archive creation */
  char c_mtime[8];    /* modification time */
  char c_filesize[8]; /* length of file in bytes */
  char c_maj[8];      /* block/char major # */
  char c_min[8];      /* block/char minor # */
  char c_rmaj[8];     /* special file major # */
  char c_rmin[8];     /* special file minor # */
  char c_namesize[8]; /* length of pathname */
  char c_chksum[8];   /* 0 OR CRC of bytes of FILE data */
} cpio_new_hdr_t;

#define CPIO_NMAGIC 070701  /* SVR4 new portable archive id */
#define CPIO_NCMAGIC 070702 /* SVR4 new portable archive id CRC */

#endif /* !_SYS_CPIO_H_ */
