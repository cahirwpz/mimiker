/* MIT/X Consortium Copyright (c) 2012 Connor Lane Smith <cls@lubutu.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>

typedef int Rune;

enum {
	UTFmax    = 6,       /* maximum bytes per rune */
	Runeself  = 0x80,    /* rune and utf are equal (<) */
	Runeerror = 0xFFFD,  /* decoding error in utf */
	Runemax   = 0x10FFFF /* maximum rune value */
};

int runetochar(char *, const Rune *);
int chartorune(Rune *, const char *);
int charntorune(Rune *, const char *, size_t);
int runelen(Rune);
size_t runenlen(const Rune *, size_t);
int fullrune(const char *, size_t);
char *utfecpy(char *, char *, const char *);
size_t utflen(const char *);
size_t utfnlen(const char *, size_t);
size_t utfmemlen(const char *, size_t);
char *utfrune(const char *, Rune);
char *utfrrune(const char *, Rune);
char *utfutf(const char *, const char *);

int isalnumrune(Rune);
int isalpharune(Rune);
int isblankrune(Rune);
int iscntrlrune(Rune);
int isdigitrune(Rune);
int isgraphrune(Rune);
int islowerrune(Rune);
int isprintrune(Rune);
int ispunctrune(Rune);
int isspacerune(Rune);
int istitlerune(Rune);
int isupperrune(Rune);
int isxdigitrune(Rune);

Rune tolowerrune(Rune);
Rune toupperrune(Rune);

size_t utftorunestr(const char *, Rune *);
size_t utfntorunestr(const char *, size_t, Rune *);

int fgetrune(Rune *, FILE *);
int efgetrune(Rune *, FILE *, const char *);
int fputrune(const Rune *, FILE *);
int efputrune(const Rune *, FILE *, const char *);
