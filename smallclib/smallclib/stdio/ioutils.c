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
* 		  file : $RCSfile: ioutils.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/**************************************************************************//**
@File         ioutils.c

@Title        IO utility functions

@Author        Imagination Technologies Ltd

@


@Platform     Any

@Synopsis   

@Description  

@DocVer       1.0 1st Release
******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <low/_stdio.h>

/*
 * Return 0 if ok to write otherwise 1.
*/
int canwrite (__smFILE *fp)
{
  /* if stream is not open for writing */
  if ((fp->_flags & __SWR) == 0)
    {
      /* if not for read-write */
      if ((fp->_flags & __SRW) == 0)
        {
          errno = EBADF;
          fp->_flags |= __SERR;
          return 1;
        }
      
      if (fp->_flags & __SRD)
        {
          /* clobber any ungetc data */
          fp->_ursize = 0;
          fp->_ucptr = 0;
          fp->_flags &= ~(__SRD | __SEOF);
          fp->_rsize = 0;
          fp->_cptr = fp->_base;
        }
        
      /* open it for writing */
      fp->_flags |= __SWR;  
      fp->_wsize = fp->_flags & __SNBF ? 0 : fp->_bsize;
    }
    
  return 0;
}/* canwrite */
