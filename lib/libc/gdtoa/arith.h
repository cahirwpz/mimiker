/* $NetBSD: arith.h,v 1.1 2006/01/25 15:33:28 kleink Exp $ */

#include <sys/endian.h>

#if BYTE_ORDER == BIG_ENDIAN
#define IEEE_BIG_ENDIAN
#else
#define IEEE_LITTLE_ENDIAN
#endif
