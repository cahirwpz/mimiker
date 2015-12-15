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
*              file : $RCSfile: signal.c,v $
*            author : $Author  Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/*
 * signal.c
 * Original Author  Imagination Technologies Ltd
 *
 * signal associates the function pointed to by func with the signal sig. When
 * a signal occurs, the value of func determines the action taken as follows:
 * if func is SIG_DFL, the default handling for that signal will occur; if func
 * is SIG_IGN, the signal will be ignored; otherwise, the default handling for
 * the signal is restored (SIG_DFL), and the function func is called with sig
 * as its argument. Returns the value of func for the previous call to signal
 * for the signal sig, or SIG_ERR if the request fails.
 */

/* _init_signal initialises the signal handlers for each signal. This function
   is called by crt0 at program startup.  */
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>

/* Included to declare getpid() */
#include <sys/unistd.h>

/* Declare kill() to suppress compilation warning */
int _EXFUN(kill, (pid_t, int));
  
typedef void (*sig_func_ptr)(int);
extern sig_func_ptr _sig_func[NSIG];

sig_func_ptr
signal (int sig, sig_func_ptr func)
{
  sig_func_ptr old_func;

  if (sig < 0 || sig >= NSIG)
    {
      errno = EINVAL;
      return SIG_ERR;
    }

  old_func = _sig_func[sig];
  _sig_func[sig] = func;

  return old_func;
}

int 
raise (int sig)
{
  sig_func_ptr func;

  if (sig < 0 || sig >= NSIG)
    {
      errno = EINVAL;
      return -1;
    }

  func = _sig_func[sig];

  if (func == SIG_DFL || func == NULL)
    return kill (getpid (), sig);
  else if (func == SIG_IGN)
    return 0;
  else if (func == SIG_ERR)
    {
      errno = EINVAL;
      return 1;
    }
  else
    {
      _sig_func[sig] = SIG_DFL;
      func (sig);
      return 0;
    }
}

