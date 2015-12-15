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
*                 file : $RCSfile: strerror.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/


#include <errno.h>
#include <string.h>

#ifdef __mips_clib_tiny

char *
strerror (int errnum)
{
  if (errnum == EDOM)
    return "Math arg out of domain of func";
  else if (errnum == ERANGE)
    return "Math result not representable";
  else if (errnum == ENOMEM)
    return "Not enough core";
  else if (errnum == EINVAL)
    return "Invalid argument";

  return NULL;
}

#else

static char *errmsg[] = {
  "(null)",
  "Not super-user",
  "No such file or directory",
  "No such process",
  "Interrupted system call",
  "I/O error",
  "No such device or address",
  "Arg list too long",
  "Exec format error",
  "Bad file number",
  "No children",
  "No more processes",
  "Not enough core",
  "Permission denied",
  "Bad address",
  "Block device required",
  "Mount device busy",
  "File exists",
  "Cross-device link",
  "No such device",
  "Not a directory",
  "Is a directory",
  "Invalid argument",
  "Too many open files in system",
  "Too many open files",
  "Not a typewriter",
  "Text file busy",
  "File too large",
  "No space left on device",
  "Illegal seek",
  "Read only file system",
  "Too many links",
  "Broken pipe",
  "Math arg out of domain of func",
  "Math result not representable",
  "No message of desired type",
  "Identifier removed",
  "Channel number out of range",
  "Level 2 not synchronized",
  "Level 3 halted",
  "Level 3 reset",
  "Link number out of range",
  "Protocol driver not attached",
  "No CSI structure available",
  "Level 2 halted",
  "Deadlock condition",
  "No record locks available",
  "Invalid exchange",
  "Invalid request descriptor",
  "Exchange full",
  "No anode",
  "Invalid request code",
  "Invalid slot",
  "File locking deadlock error",
  "Bad font file fmt",
  "Device not a stream",
  "No data (for no delay io)",
  "Timer expired",
  "Out of streams resources",
  "Machine is not on the network",
  "Package not installed",
  "The object is remote",
  "The link has been severed",
  "Advertise error",
  "Srmount error",
  "Communication error on send",
  "Protocol error",
  "Multihop attempted",
  "Inode is remote (not really error)",
  "Cross mount point (not really error)",
  "Trying to read unreadable message",
  "Inappropriate file type or format",
  "Given log. name not unique",
  "f.d. invalid for this operation",
  "Remote address changed",
  "Can't access a needed shared lib",
  "Accessing a corrupted shared lib",
  ".lib section in a.out corrupted",
  "Attempting to link in too many libs",
  "Attempting to exec a shared library",
  "Function not implemented",
  "No more files",
  "Directory not empty",
  "File or path name too long",
  "Too many symbolic links",
  "Operation not supported on transport endpoint",
  "Protocol family not supported",
  "Connection reset by peer",
  "No buffer space available",
  "Address family not supported by protocol family",
  "Protocol wrong type for socket",
  "Socket operation on non-socket",
  "Protocol not available",
  "Can't send after socket shutdown",
  "Connection refused",
  "Address already in use",
  "Connection aborted",
  "Network is unreachable",
  "Network interface is not configured",
  "Connection timed out",
  "Host is down",
  "Host is unreachable",
  "Connection already in progress",
  "Socket already connected",
  "Destination address required",
  "Message too long",
  "Unknown protocol",
  "Socket type not supported",
  "Address not available",
  "(null)",
  "Socket is already connected",
  "Socket is not connected",
  "(null)",
  "(null)",
  "(null)",
  "(null)",
  "(null)",
  "Not supported",
  "No medium (in tape drive)",
  "No such host or network path",
  "Filename exists with different case",
  "(null)",
  "Value too large for defined data type",
  "Operation canceled",
};
  
char *
strerror(int errnum)
{
  char *error=NULL;
  	
  if (errnum >= EPERM && errnum <= ECANCELED)
    error = errmsg[errnum];
  		
  return error;
}
 
#endif

