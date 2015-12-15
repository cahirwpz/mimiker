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
*                 file : $RCSfile: _stdlib.h,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*****************************************************************************//**
Description  Macros used in the stdlib of SmallLib
********************************************************************************/

#ifndef _LOW_STDLIB_H_
#define _LOW_STDLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __mbstate_t_defined
#include <wchar.h>
#endif

enum lctype {iso, cp, utf, sjis, eucjp, jis, ascii};

char *__locale_charset (void);
char *_gcvt (double , int , char *, char, int);
int __common_mbtowc (wchar_t *pwc, const char *s, size_t n, const char *charset, mbstate_t *state);
int __common_wctomb (char *s, wchar_t _wchar, const char *charset, mbstate_t *state);
int __iso_8859_index (const char *);
int __cp_index (const char *);
wint_t _jp2uc (wint_t);   /* internal function to translate JP to Unicode */

extern enum lctype __lc_wctomb;
extern enum lctype __lc_mbtowc;
extern wchar_t __iso_8859_conv[14][0x60];
extern wchar_t __cp_conv[][0x80];

/* escape character used for JIS encoding */
#define ESC_CHAR 0x1b

/* functions used to support SHIFT_JIS, EUC-JP, and JIS multibyte encodings */
int _issjis1 (int c);
int _issjis2 (int c);
int _iseucjp1 (int c);
int _iseucjp2 (int c);
int _isjis (int c);

#define _issjis1(c)    (((c) >= 0x81 && (c) <= 0x9f) || ((c) >= 0xe0 && (c) <= 0xef))
#define _issjis2(c)    (((c) >= 0x40 && (c) <= 0x7e) || ((c) >= 0x80 && (c) <= 0xfc))
#define _iseucjp1(c)   ((c) == 0x8e || (c) == 0x8f || ((c) >= 0xa1 && (c) <= 0xfe))
#define _iseucjp2(c)   ((c) >= 0xa1 && (c) <= 0xfe)
#define _isjis(c)      ((c) >= 0x21 && (c) <= 0x7e)

/* valid values for wctrans_t */
#define WCT_TOLOWER 1
#define WCT_TOUPPER 2

/* valid values for wctype_t */
#define WC_ALNUM	1
#define WC_ALPHA	2
#define WC_BLANK	3
#define WC_CNTRL	4
#define WC_DIGIT	5
#define WC_GRAPH	6
#define WC_LOWER	7
#define WC_PRINT	8
#define WC_PUNCT	9
#define WC_SPACE	10
#define WC_UPPER	11
#define WC_XDIGIT	12

#ifdef __cplusplus
}
#endif

#endif /* _LOW_STDLIB_H_ */

