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
* 		  file : $RCSfile: mktemp.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
/* This is file MKTEMP.C */
/* This file may have been modified by DJ Delorie (Jan 1991).  If so,
** these modifications are Copyright (C) 1991 DJ Delorie.
*/

/* Generate unused directory/file name */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

static int _gettemp (char *path, int *doopen, 
        int domkdir, size_t suffixlen, int flags)
{
  struct stat sbuf;
  unsigned int pid;
  char *start, *trv, *end;

  pid = getpid ();
  
  /* extra X's get set to 0's */
  for (trv = path; *trv; ++trv)
    continue;
    
  if (trv - path < suffixlen)
    {
      errno = EINVAL;
      return 0;
    }
    
  trv -= suffixlen;
  end = trv;
  
  while (path < trv && *--trv == 'X')
    {
      *trv = (pid % 10) + '0';
      pid /= 10;
    }
    
  if (end - trv < 6)
    {
      errno = EINVAL;
      return 0;
    }

  /*
   * Check the target directory; if you have six X's and it
   * doesn't exist this runs for a *very* long time.
   */

  for (start = trv + 1;; --trv)
    {
      if (trv <= path)
        break;
      if (*trv == '/')
        {
          *trv = '\0';
          if (stat (path, &sbuf))
            return (0);
          if (!(sbuf.st_mode & S_IFDIR))
            {
              errno = ENOTDIR;
              return (0);
            }
          *trv = '/';
          break;
        }
    }

  for (;;)
    {
      if (domkdir)
        {
          errno = ENOSYS;
          return 0;
        }
      else if (doopen)
        {
          if ((*doopen = open (path, O_CREAT | O_EXCL | O_RDWR | flags, 0600)) >= 0)
            return 1;
          if (errno != EEXIST)
            return 0;
        }
      else if (stat (path, &sbuf))
        return (errno == ENOENT ? 1 : 0);

      /* tricky little algorithm for backward compatibility */
      for (trv = start;;)
        {
          if (trv == end)
            return 0;
          if (*trv == 'z')
            *trv++ = 'a';
          else
            {
              /* Safe, since it only encounters 7-bit characters.  */
              if (isdigit ((unsigned char) *trv))
                *trv = 'a';
              else
                ++ * trv;
              break;
            }
        }
    }
    
  /*NOTREACHED*/
  return 0;
}/* _gettemp */

#ifndef O_BINARY
#define O_BINARY 0
#endif

int mkstemp (char *path)
{
  int fd;
  return (_gettemp (path, &fd, 0, 0, O_BINARY) ? fd : -1);
}/* mkstemp */

char * mkdtemp (char *path)
{
  return (_gettemp (path, NULL, 1, 0, 0) ? path : NULL);
}/* mkdtemp */

int mkstemps (char *path, int len)
{
  int fd;
  return (_gettemp (path, &fd, 0, len, O_BINARY) ? fd : -1);
}/* mkstemps */

int mkostemp (char *path, int flags)
{
  int fd;
  return (_gettemp (path, &fd, 0, 0, flags & ~O_ACCMODE) ? fd : -1);
}/* mkostemp */

int mkostemps (char *path, int len, int flags)
{
  int fd;
  return (_gettemp (path, &fd, 0, len, flags & ~O_ACCMODE) ? fd : -1);
}/* mkostemps */

char * mktemp (char *path)
{
  return (_gettemp (path, NULL, 0, 0, 0) ? path : NULL);
}/* mktemp */

