#ifndef _SYS_ENDIAN_H_
#define _SYS_ENDIAN_H_

#ifndef __BYTE_ORDER__
#error __BYTE_ORDER__ has not been defined by the compiler!
#endif

#define BIG_ENDIAN __ORDER_BIG_ENDIAN__
#define LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#define PDP_ENDIAN __ORDER_PDP_ENDIAN__
#define BYTE_ORDER __BYTE_ORDER__

#endif /* !_SYS_ENDIAN_H_ */
