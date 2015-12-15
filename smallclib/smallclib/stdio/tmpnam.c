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
* 		  file : $RCSfile: tmpnam.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <low/_stdio.h>

static char __tmpnam_buf[L_tmpnam] = {0,};   /* buffer for name of temporary file */
static int __inc = 0;                   /* sequence number */

/* Small int to ascii converter */
static void __itoa (char *ptr, unsigned int num)
{
  int ch;

  do
  {
    ch = num & 0xf;
    ch = ch >= 10 ? (ch - 10) + 'a' : ch + '0';
    *ptr++ =  ch;
    num >>= 4;
  } while (num != 0);

  *ptr = 0;
  
  return;
}/* __itoa */

/* 
 * Return 1 if specified file could be opened, otherwise 0.  
*/
static int worker (char *result, const char *part1, 
                    const char *part2, int part3, int *part4)
{
  int t, len;
  char p3buf[10] = {0,};
  
  __itoa (p3buf, part3);
  
  while (1)
  {
    strcpy (result, part1);
    strcat (result, "/");
    strcat (result, part2);
    strcat (result, p3buf);
    strcat (result, ".");
    len = strlen (result);
    __itoa (result + len, *part4);
    (*part4)++;
    
    t = open (result, O_RDONLY, 0);
    if (t == -1)
    {
      if (errno == ENOSYS)
      {
        result[0] = '\0';
        return 0;
      }
      break;
    }
    close (t);
  }
  
  return 1;
}/* worker */

/*
 * Name for a temporary file
*/
char *tmpnam (char *s)
{
  int pid;
  
  if (s == NULL)
    s = __tmpnam_buf;
  
  pid = getpid ();
  
  if (worker (s, P_tmpdir, "t", pid, &__inc))
    {
      __inc++;
      return s;
    }
  
  return NULL;
}/* tmpnam */

