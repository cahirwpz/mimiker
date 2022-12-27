#ifndef _COMMON_INT_FMTIO_H_
#define _COMMON_INT_FMTIO_H_

#ifndef __INT8_FMTd__
#define __INT8_FMTd__ "d"
#endif
#ifndef __INT16_FMTd__
#define __INT16_FMTd__ "d"
#endif
#ifndef __INT32_FMTd__
#define __INT32_FMTd__ "d"
#endif
#ifndef __INT64_FMTd__
#ifndef _LP64
#define __INT64_FMTd__ "lld"
#else
#define __INT64_FMTd__ "ld"
#endif
#endif

#ifndef __INT_LEAST8_FMTd__
#define __INT_LEAST8_FMTd__ __INT8_FMTd__
#endif
#ifndef __INT_LEAST16_FMTd__
#define __INT_LEAST16_FMTd__ __INT16_FMTd__
#endif
#ifndef __INT_LEAST32_FMTd__
#define __INT_LEAST32_FMTd__ __INT32_FMTd__
#endif
#ifndef __INT_LEAST64_FMTd__
#define __INT_LEAST64_FMTd__ __INT64_FMTd__
#endif
#ifndef __INT_FAST8_FMTd__
#define __INT_FAST8_FMTd__ __INT32_FMTd__
#endif
#ifndef __INT_FAST16_FMTd__
#define __INT_FAST16_FMTd__ __INT32_FMTd__
#endif
#ifndef __INT_FAST32_FMTd__
#define __INT_FAST32_FMTd__ __INT32_FMTd__
#endif
#ifndef __INT_FAST64_FMTd__
#define __INT_FAST64_FMTd__ __INT64_FMTd__
#endif
#ifndef __INTMAX_FMTd__
#define __INTMAX_FMTd__ __INT64_FMTd__
#endif
#ifndef __INTPTR_FMTd__
#ifndef _LP64
#define __INTPTR_FMTd__ __INT32_FMTd__
#else
#define __INTPTR_FMTd__ __INT64_FMTd__
#endif
#endif

#ifndef __INT8_FMTi__
#define __INT8_FMTi__ "i"
#endif
#ifndef __INT16_FMTi__
#define __INT16_FMTi__ "i"
#endif
#ifndef __INT32_FMTi__
#define __INT32_FMTi__ "i"
#endif
#ifndef __INT64_FMTi__
#ifndef _LP64
#define __INT64_FMTi__ "lli"
#else
#define __INT64_FMTi__ "li"
#endif
#endif

#ifndef __INT_LEAST8_FMTi__
#define __INT_LEAST8_FMTi__ __INT8_FMTi__
#endif
#ifndef __INT_LEAST16_FMTi__
#define __INT_LEAST16_FMTi__ __INT16_FMTi__
#endif
#ifndef __INT_LEAST32_FMTi__
#define __INT_LEAST32_FMTi__ __INT32_FMTi__
#endif
#ifndef __INT_LEAST64_FMTi__
#define __INT_LEAST64_FMTi__ __INT64_FMTi__
#endif
#ifndef __INT_FAST8_FMTi__
#define __INT_FAST8_FMTi__ __INT32_FMTi__
#endif
#ifndef __INT_FAST16_FMTi__
#define __INT_FAST16_FMTi__ __INT32_FMTi__
#endif
#ifndef __INT_FAST32_FMTi__
#define __INT_FAST32_FMTi__ __INT32_FMTi__
#endif
#ifndef __INT_FAST64_FMTi__
#define __INT_FAST64_FMTi__ __INT64_FMTi__
#endif
#ifndef __INTMAX_FMTi__
#define __INTMAX_FMTi__ __INT64_FMTi__
#endif
#ifndef __INTPTR_FMTi__
#ifndef _LP64
#define __INTPTR_FMTi__ __INT32_FMTi__
#else
#define __INTPTR_FMTi__ __INT64_FMTi__
#endif
#endif

#ifndef __UINT8_FMTo__
#define __UINT8_FMTo__ "o"
#endif
#ifndef __UINT16_FMTo__
#define __UINT16_FMTo__ "o"
#endif
#ifndef __UINT32_FMTo__
#define __UINT32_FMTo__ "o"
#endif
#ifndef __UINT64_FMTo__
#ifndef _LP64
#define __UINT64_FMTo__ "llo"
#else
#define __UINT64_FMTo__ "lo"
#endif
#endif

#ifndef __UINT_LEAST8_FMTo__
#define __UINT_LEAST8_FMTo__ __UINT8_FMTo__
#endif
#ifndef __UINT_LEAST16_FMTo__
#define __UINT_LEAST16_FMTo__ __UINT16_FMTo__
#endif
#ifndef __UINT_LEAST32_FMTo__
#define __UINT_LEAST32_FMTo__ __UINT32_FMTo__
#endif
#ifndef __UINT_LEAST64_FMTo__
#define __UINT_LEAST64_FMTo__ __UINT64_FMTo__
#endif
#ifndef __UINT_FAST8_FMTo__
#define __UINT_FAST8_FMTo__ __UINT32_FMTo__
#endif
#ifndef __UINT_FAST16_FMTo__
#define __UINT_FAST16_FMTo__ __UINT32_FMTo__
#endif
#ifndef __UINT_FAST32_FMTo__
#define __UINT_FAST32_FMTo__ __UINT32_FMTo__
#endif
#ifndef __UINT_FAST64_FMTo__
#define __UINT_FAST64_FMTo__ __UINT64_FMTo__
#endif
#ifndef __UINTMAX_FMTo__
#define __UINTMAX_FMTo__ __UINT64_FMTo__
#endif
#ifndef __UINTPTR_FMTo__
#ifndef _LP64
#define __UINTPTR_FMTo__ __UINT32_FMTo__
#else
#define __UINTPTR_FMTo__ __UINT64_FMTo__
#endif
#endif

#ifndef __UINT8_FMTx__
#define __UINT8_FMTx__ "x"
#endif
#ifndef __UINT16_FMTx__
#define __UINT16_FMTx__ "x"
#endif
#ifndef __UINT32_FMTx__
#define __UINT32_FMTx__ "x"
#endif
#ifndef __UINT64_FMTx__
#ifndef _LP64
#define __UINT64_FMTx__ "llx"
#else
#define __UINT64_FMTx__ "lx"
#endif
#endif

#ifndef __UINT_LEAST8_FMTx__
#define __UINT_LEAST8_FMTx__ __UINT8_FMTx__
#endif
#ifndef __UINT_LEAST16_FMTx__
#define __UINT_LEAST16_FMTx__ __UINT16_FMTx__
#endif
#ifndef __UINT_LEAST32_FMTx__
#define __UINT_LEAST32_FMTx__ __UINT32_FMTx__
#endif
#ifndef __UINT_LEAST64_FMTx__
#define __UINT_LEAST64_FMTx__ __UINT64_FMTx__
#endif
#ifndef __UINT_FAST8_FMTx__
#define __UINT_FAST8_FMTx__ __UINT32_FMTx__
#endif
#ifndef __UINT_FAST16_FMTx__
#define __UINT_FAST16_FMTx__ __UINT32_FMTx__
#endif
#ifndef __UINT_FAST32_FMTx__
#define __UINT_FAST32_FMTx__ __UINT32_FMTx__
#endif
#ifndef __UINT_FAST64_FMTx__
#define __UINT_FAST64_FMTx__ __UINT64_FMTx__
#endif
#ifndef __UINTMAX_FMTx__
#define __UINTMAX_FMTx__ __UINT64_FMTx__
#endif
#ifndef __UINTPTR_FMTx__
#ifndef _LP64
#define __UINTPTR_FMTx__ __UINT32_FMTx__
#else
#define __UINTPTR_FMTx__ __UINT64_FMTx__
#endif
#endif

#ifndef __UINT8_FMTX__
#define __UINT8_FMTX__ "X"
#endif
#ifndef __UINT16_FMTX__
#define __UINT16_FMTX__ "X"
#endif
#ifndef __UINT32_FMTX__
#define __UINT32_FMTX__ "X"
#endif
#ifndef __UINT64_FMTX__
#ifndef _LP64
#define __UINT64_FMTX__ "llX"
#else
#define __UINT64_FMTX__ "lX"
#endif
#endif

#ifndef __UINT_LEAST8_FMTX__
#define __UINT_LEAST8_FMTX__ __UINT8_FMTX__
#endif
#ifndef __UINT_LEAST16_FMTX__
#define __UINT_LEAST16_FMTX__ __UINT16_FMTX__
#endif
#ifndef __UINT_LEAST32_FMTX__
#define __UINT_LEAST32_FMTX__ __UINT32_FMTX__
#endif
#ifndef __UINT_LEAST64_FMTX__
#define __UINT_LEAST64_FMTX__ __UINT64_FMTX__
#endif
#ifndef __UINT_FAST8_FMTX__
#define __UINT_FAST8_FMTX__ __UINT32_FMTX__
#endif
#ifndef __UINT_FAST16_FMTX__
#define __UINT_FAST16_FMTX__ __UINT32_FMTX__
#endif
#ifndef __UINT_FAST32_FMTX__
#define __UINT_FAST32_FMTX__ __UINT32_FMTX__
#endif
#ifndef __UINT_FAST64_FMTX__
#define __UINT_FAST64_FMTX__ __UINT64_FMTX__
#endif
#ifndef __UINTMAX_FMTX__
#define __UINTMAX_FMTX__ __UINT64_FMTX__
#endif
#ifndef __UINTPTR_FMTX__
#ifndef _LP64
#define __UINTPTR_FMTX__ __UINT32_FMTX__
#else
#define __UINTPTR_FMTX__ __UINT64_FMTX__
#endif
#endif

#endif /* !_COMMON_INT_FMTIO_H_ */
