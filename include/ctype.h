/*	$OpenBSD: ctype.h,v 1.24 2014/05/26 01:49:36 guenther Exp $	*/
/*	$NetBSD: ctype.h,v 1.14 1994/10/26 00:55:47 cgd Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ctype.h	5.3 (Berkeley) 4/3/91
 */

#ifndef _CTYPE_H_
#define _CTYPE_H_

/*
 * Macro to test if we're using a specific version of gcc or later.
 */
#undef __GNUC_PREREQ__
#ifdef __GNUC__
#define __GNUC_PREREQ__(ma, mi) \
	((__GNUC__ > (ma)) || (__GNUC__ == (ma) && __GNUC_MINOR__ >= (mi)))
#else
#define __GNUC_PREREQ__(ma, mi) 0
#endif

/*
 * __only_inline makes the compiler only use this function definition
 * for inlining; references that can't be inlined will be left as
 * external references instead of generating a local copy.  The
 * matching library should include a simple extern definition for
 * the function to handle those references.  c.f. ctype.h
 */
#ifdef __GNUC__
#  if __GNUC_PREREQ__(4, 2)
#define __only_inline	extern __inline __attribute__((__gnu_inline__))
#  else
#define __only_inline	extern __inline
#  endif
#else
#define __only_inline	static __inline
#endif

#define	_U	0x01
#define	_L	0x02
#define	_N	0x04
#define	_S	0x08
#define	_P	0x10
#define	_C	0x20
#define	_X	0x40
#define	_B	0x80

extern const char	*_ctype_;
extern const short	*_tolower_tab_;
extern const short	*_toupper_tab_;

#if defined(__GNUC__) || defined(_ANSI_LIBRARY)
int	isalnum(int);
int	isalpha(int);
int	iscntrl(int);
int	isdigit(int);
int	isgraph(int);
int	islower(int);
int	isprint(int);
int	ispunct(int);
int	isspace(int);
int	isupper(int);
int	isxdigit(int);
int	tolower(int);
int	toupper(int);

int	isblank(int);

int	isascii(int);
int	toascii(int);
int	_tolower(int);
int	_toupper(int);

#endif /* __GNUC__ || _ANSI_LIBRARY */

#if !defined(_ANSI_LIBRARY)

__only_inline int isalnum(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & (_U|_L|_N)));
}

__only_inline int isalpha(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & (_U|_L)));
}

__only_inline int iscntrl(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & _C));
}

__only_inline int isdigit(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & _N));
}

__only_inline int isgraph(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & (_P|_U|_L|_N)));
}

__only_inline int islower(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & _L));
}

__only_inline int isprint(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & (_P|_U|_L|_N|_B)));
}

__only_inline int ispunct(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & _P));
}

__only_inline int isspace(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & _S));
}

__only_inline int isupper(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & _U));
}

__only_inline int isxdigit(int _c)
{
	return (_c == -1 ? 0 : ((_ctype_ + 1)[(unsigned char)_c] & (_N|_X)));
}

__only_inline int tolower(int _c)
{
	if ((unsigned int)_c > 255)
		return (_c);
	return ((_tolower_tab_ + 1)[_c]);
}

__only_inline int toupper(int _c)
{
	if ((unsigned int)_c > 255)
		return (_c);
	return ((_toupper_tab_ + 1)[_c]);
}

__only_inline int isblank(int _c)
{
	return (_c == ' ' || _c == '\t');
}

__only_inline int isascii(int _c)
{
	return ((unsigned int)_c <= 0177);
}

__only_inline int toascii(int _c)
{
	return (_c & 0177);
}

__only_inline int _tolower(int _c)
{
	return (_c - 'A' + 'a');
}

__only_inline int _toupper(int _c)
{
	return (_c - 'a' + 'A');
}

#endif /* !_ANSI_LIBRARY */

#endif /* !_CTYPE_H_ */
