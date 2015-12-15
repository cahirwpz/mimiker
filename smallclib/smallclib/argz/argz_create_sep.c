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
*              file : $RCSfile: argz_create_sep.c,v $
*            author : $Author  Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* Copyright (C) 2002 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

/* same as newlib 2.0 */

#include <argz.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

error_t
_DEFUN (argz_create_sep, (string, sep, argz, argz_len),
       const char *string _AND
       int sep _AND
       char **argz _AND
       size_t *argz_len)
{
  int len = 0;
  int i = 0;
  int num_strings = 0;
  char delim[2];
  char *running = 0;
  char *old_running = 0;
  char *token = 0;
  char *iter = 0;

  *argz_len = 0;

  if (!string || string[0] == '\0')
    {
      *argz= NULL;
      return 0;
    }

  delim[0] = sep;
  delim[1] = '\0';

  running = strdup(string);
  old_running = running;

  while ((token = strsep(&running, delim)))
    {
      len = strlen(token);
      *argz_len += (len + 1);
      num_strings++;
    }

  if(!(*argz = (char *)malloc(*argz_len)))
    return ENOMEM;

  free(old_running);

  running = strdup(string);
  old_running = running;

  iter = *argz;
  for (i = 0; i < num_strings; i++)
    {
      token = strsep(&running, delim);
      len = strlen(token) + 1;
      memcpy(iter, token, len);
      iter += len;
    }

  free(old_running);
  return 0;
}
