/* $OpenBSD: ctype_private.h,v 1.2 2015/08/27 04:37:09 guenther Exp $ */
/* Written by Marc Espie, public domain */
#define CTYPE_NUM_CHARS       256
#define EOF (-1)

#define	_U	0x01
#define	_L	0x02
#define	_N	0x04
#define	_S	0x08
#define	_P	0x10
#define	_C	0x20
#define	_X	0x40
#define	_B	0x80
