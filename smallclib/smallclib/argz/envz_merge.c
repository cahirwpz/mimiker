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
*              file : $RCSfile: envz_merge.c,v $
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

#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <argz.h>
#include <envz.h>

error_t
_DEFUN (envz_merge, (envz, envz_len, envz2, envz2_len, override),
       char **envz _AND
       size_t *envz_len _AND
       const char *envz2 _AND
       size_t envz2_len _AND
       int override)
{
  char *entry = NULL;
  char *name_str = NULL;
  char *val_str = NULL;
  char *name_iter = NULL;
  int retval = 0;

  while((entry = argz_next((char *)envz2, envz2_len, entry)) && !retval)
    {
      if (!override)
        {
          name_str = strdup (entry);
          name_iter = strchr(name_str, '=');
          if(name_iter)
            *name_iter = '\0';

          if(!envz_entry(*envz, *envz_len, name_str))
            {
              retval = argz_add(envz, envz_len, entry);
            }
          free(name_str);
        }
      else
        {
          name_str = strdup (entry);
          name_iter = strchr(name_str, '=');
          if(name_iter)
            {
              *name_iter = '\0';
              val_str = name_iter + 1;
            }
          else
            {
              val_str = NULL;
            }

          retval = envz_add(envz, envz_len, name_str, val_str);
        }
    }
  return retval;
}
