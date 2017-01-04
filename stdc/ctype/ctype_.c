/*	$OpenBSD: ctype_.c,v 1.12 2015/09/19 04:02:21 guenther Exp $ */
/*	$OpenBSD: isctype.c,v 1.12 2015/09/13 11:38:08 guenther Exp $ */
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
 */

#include "ctype_private.h"

static const char _ctype_[1 + CTYPE_NUM_CHARS] = {
	0,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
   _S|(char)_B,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
	_N,	_N,	_N,	_N,	_N,	_N,	_N,	_N,
	_N,	_N,	_P,	_P,	_P,	_P,	_P,	_P,
	_P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_P,	_P,	_P,	_P,	_P,
	_P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_P,	_P,	_P,	_P,	_C,

	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* 80 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* 88 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* 90 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* 98 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* A0 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* A8 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* B0 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* B8 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* C0 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* C8 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* D0 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* D8 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* E0 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* E8 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0, /* F0 */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0  /* F8 */
};


int isalnum(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_U|_L|_N)));
}

int isalpha(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_U|_L)));
}

int isblank(int c) {
	return (c == ' ' || c == '\t');
}

int iscntrl(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _C));
}

int isdigit(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _N));
}

int isgraph(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_P|_U|_L|_N)));
}

int islower(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _L));
}

int isprint(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_P|_U|_L|_N|_B)));
}

int ispunct(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _P));
}

int isspace(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _S));
}

int isupper(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & _U));
}

int isxdigit(int c) {
	return (c == EOF ? 0 : ((_ctype_ + 1)[(unsigned char)c] & (_N|_X)));
}

int isascii(int c) {
	return ((unsigned int)c <= 0177);
}

int toascii(int c) {
	return (c & 0177);
}
