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
*              file : $RCSfile: mlock.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* same as newlib2.0 */

#ifndef MALLOC_PROVIDED
/*
FUNCTION
<<__malloc_lock>>, <<__malloc_unlock>>---lock malloc pool

INDEX
	__malloc_lock
INDEX
	__malloc_unlock

ANSI_SYNOPSIS
	#include <malloc.h>
	void __malloc_lock (struct _reent *<[reent]>);
	void __malloc_unlock (struct _reent *<[reent]>);

TRAD_SYNOPSIS
	void __malloc_lock(<[reent]>)
	struct _reent *<[reent]>;

	void __malloc_unlock(<[reent]>)
	struct _reent *<[reent]>;

DESCRIPTION
The <<malloc>> family of routines call these functions when they need to lock
the memory pool.  The version of these routines supplied in the library use
the lock API defined in sys/lock.h.  If multiple threads of execution can
call <<malloc>>, or if <<malloc>> can be called reentrantly, then you need to
define your own versions of these functions in order to safely lock the
memory pool during a call.  If you do not, the memory pool may become
corrupted.

A call to <<malloc>> may call <<__malloc_lock>> recursively; that is,
the sequence of calls may go <<__malloc_lock>>, <<__malloc_lock>>,
<<__malloc_unlock>>, <<__malloc_unlock>>.  Any implementation of these
routines must be careful to avoid causing a thread to wait for a lock
that it already holds.
*/

#include <malloc.h>
#include <sys/lock.h>
#include "low/_lock.h"

#ifndef __SINGLE_THREAD__
__LOCK_INIT_RECURSIVE(static, __malloc_lock_object);
#endif

void
__malloc_lock (ptr)
     struct _reent *ptr;
{
#ifndef __SINGLE_THREAD__
  __lock_acquire_recursive (__malloc_lock_object);
#endif
}

void
__malloc_unlock (ptr)
     struct _reent *ptr;
{
#ifndef __SINGLE_THREAD__
  __lock_release_recursive (__malloc_lock_object);
#endif
}

#endif
