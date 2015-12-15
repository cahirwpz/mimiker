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
*                 file : $RCSfile: setenv.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

extern char **environ;

/* Only deal with a pointer to environ, to work around subtle bugs with shared
   libraries and/or small data systems where the user declares his own
   'environ'.  */
static char ***p_environ = &environ;

/* _findenv_r is defined in getenv_r.c.  */
extern char *__findenv (const char *, int *);

/*
 * setenv --
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 *	If "name" contains equal sign, -1 is returned, and errno is
 *	set to EINVAL;
 */

int
setenv (const char *name, const char *value, int rewrite)
{
  register char *C;
  int l_value, offset;
  register int cnt;

  if (strchr(name, '='))
    {
      errno = EINVAL;
      return -1;
    }

  l_value = strlen (value);
  if ((C = __findenv (name, &offset)))
    {				/* find if already exists */
      if (!rewrite)
	    return 0;
      if (strlen (C) >= l_value)
	    {			/* old larger; copy over */
	      while ((*C++ = *value++));
	      return 0;
	    }
    }
  else
    {				/* create new slot */
      register char **P;

      for (P = *p_environ, cnt = 0; *P; ++P, ++cnt);

      P = (char **) malloc ((size_t) (sizeof (char *) * (cnt + 2)));

      if (!P)
        return (-1);

      memcpy((char *) P,(char *) *p_environ, cnt * sizeof (char *));
      *p_environ = P;
      (*p_environ)[cnt + 1] = NULL;
      offset = cnt;
    }

  if (!((*p_environ)[offset] =	/* name + `=' + value */
	  malloc ((size_t) (strlen (name) + l_value + 2))))
	  return -1;
  for (C = (*p_environ)[offset]; (*C = *name++) && *C != '='; ++C);
  for (*C++ = '='; (*C++ = *value++) != 0;);
  return 0;
}

/*
 * unsetenv(name) --
 *	Delete environmental variable "name".
 */
int
unsetenv (const char *name)
{
  register char **P;
  int offset;

  /* Name cannot be NULL, empty, or contain an equal sign.  */
  if (name == NULL || name[0] == '\0' || strchr(name, '='))
    {
      errno = EINVAL;
      return -1;
    }

  while (__findenv (name, &offset))	/* if set multiple times */
    {
      for (P = &(*p_environ)[offset];; ++P)
        if (!(*P = *(P + 1)))
	      break;
    }
  return 0;
}
