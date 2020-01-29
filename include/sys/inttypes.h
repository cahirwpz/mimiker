#ifndef _SYS_INTTYPES_H_
#define _SYS_INTTYPES_H_

#include <machine/int_fmtio.h>

/*
 * 7.8.1 Macros for format specifiers
 */

/* fprintf macros for signed integers */
#define PRId8 __INT8_FMTd__
#define PRId16 __INT16_FMTd__
#define PRId32 __INT32_FMTd__
#define PRId64 __INT64_FMTd__
#define PRIdLEAST8 __INT_LEAST8_FMTd__
#define PRIdLEAST16 __INT_LEAST16_FMTd__
#define PRIdLEAST32 __INT_LEAST32_FMTd__
#define PRIdLEAST64 __INT_LEAST64_FMTd__
#define PRIdFAST8 __INT_FAST8_FMTd__
#define PRIdFAST16 __INT_FAST16_FMTd__
#define PRIdFAST32 __INT_FAST32_FMTd__
#define PRIdFAST64 __INT_FAST64_FMTd__
#define PRIdMAX __INTMAX_FMTd__
#define PRIdPTR __INTPTR_FMTd__

#define PRIi8 __INT8_FMTi__
#define PRIi16 __INT16_FMTi__
#define PRIi32 __INT32_FMTi__
#define PRIi64 __INT64_FMTi__
#define PRIiLEAST8 __INT_LEAST8_FMTi__
#define PRIiLEAST16 __INT_LEAST16_FMTi__
#define PRIiLEAST32 __INT_LEAST32_FMTi__
#define PRIiLEAST64 __INT_LEAST64_FMTi__
#define PRIiFAST8 __INT_FAST8_FMTi__
#define PRIiFAST16 __INT_FAST16_FMTi__
#define PRIiFAST32 __INT_FAST32_FMTi__
#define PRIiFAST64 __INT_FAST64_FMTi__
#define PRIiMAX __INTMAX_FMTi__
#define PRIiPTR __INTPTR_FMTi__

/* fprintf macros for unsigned integers */
#define PRIo8 __UINT8_FMTo__
#define PRIo16 __UINT16_FMTo__
#define PRIo32 __UINT32_FMTo__
#define PRIo64 __UINT64_FMTo__
#define PRIoLEAST8 __UINT_LEAST8_FMTo__
#define PRIoLEAST16 __UINT_LEAST16_FMTo__
#define PRIoLEAST32 __UINT_LEAST32_FMTo__
#define PRIoLEAST64 __UINT_LEAST64_FMTo__
#define PRIoFAST8 __UINT_FAST8_FMTo__
#define PRIoFAST16 __UINT_FAST16_FMTo__
#define PRIoFAST32 __UINT_FAST32_FMTo__
#define PRIoFAST64 __UINT_FAST64_FMTo__

#endif /* !_SYS_INTTYPES_H_ */
