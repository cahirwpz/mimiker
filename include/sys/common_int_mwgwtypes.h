#ifndef _COMMON_INT_MWGWTYPES_H_
#define _COMMON_INT_MWGWTYPES_H_

/*
 * 7.18.1 Integer types
 */

/* 7.18.1.1 Exact-width integer types */

#ifndef __INT8_TYPE__
#define __INT8_TYPE__ signed char
#endif
#ifndef __UINT8_TYPE__
#define __UINT8_TYPE__ unsigned char
#endif
#ifndef __INT16_TYPE__
#define __INT16_TYPE__ short signed int
#endif
#ifndef __UINT16_TYPE__
#define __UINT16_TYPE__ short unsigned int
#endif
#ifndef __INT32_TYPE__
#define __INT32_TYPE__ signed int
#endif
#ifndef __UINT32_TYPE__
#define __UINT32_TYPE__ unsigned int
#endif
#ifndef _LP64
#ifndef __INT64_TYPE__
#define __INT64_TYPE__ long long signed int
#endif
#ifndef __UINT64_TYPE__
#define __UINT64_TYPE__ long long unsigned int
#endif
#else
#ifndef __INT64_TYPE__
#define __INT64_TYPE__ long signed int
#endif
#ifndef __UINT64_TYPE__
#define __UINT64_TYPE__ long unsigned int
#endif
#endif

/* 7.18.1.2 Minimum-width integer types */

#ifndef __INT_LEAST8_TYPE__
#define __INT_LEAST8_TYPE__ __INT8_TYPE__
#endif
#ifndef __UINT_LEAST8_TYPE__
#define __UINT_LEAST8_TYPE__ __UINT8_TYPE__
#endif
#ifndef __INT_LEAST16_TYPE__
#define __INT_LEAST16_TYPE__ __INT16_TYPE__
#endif
#ifndef __UINT_LEAST16_TYPE__
#define __UINT_LEAST16_TYPE__ __UINT16_TYPE__
#endif
#ifndef __INT_LEAST32_TYPE__
#define __INT_LEAST32_TYPE__ __INT32_TYPE__
#endif
#ifndef __UINT_LEAST32_TYPE__
#define __UINT_LEAST32_TYPE__ __UINT32_TYPE__
#endif
#ifndef __INT_LEAST64_TYPE__
#define __INT_LEAST64_TYPE__ __INT64_TYPE__
#endif
#ifndef __UINT_LEAST64_TYPE__
#define __UINT_LEAST64_TYPE__ __UINT64_TYPE__
#endif

/* 7.18.1.3 Fastest minimum-width integer types */

#ifndef __INT_FAST8_TYPE__
#define __INT_FAST8_TYPE__ __INT32_TYPE__
#endif
#ifndef __UINT_FAST8_TYPE__
#define __UINT_FAST8_TYPE__ __UINT32_TYPE__
#endif
#ifndef __INT_FAST16_TYPE__
#define __INT_FAST16_TYPE__ __INT32_TYPE__
#endif
#ifndef __UINT_FAST16_TYPE__
#define __UINT_FAST16_TYPE__ __UINT32_TYPE__
#endif
#ifndef __INT_FAST32_TYPE__
#define __INT_FAST32_TYPE__ __INT32_TYPE__
#endif
#ifndef __UINT_FAST32_TYPE__
#define __UINT_FAST32_TYPE__ __UINT32_TYPE__
#endif
#ifndef __INT_FAST64_TYPE__
#define __INT_FAST64_TYPE__ __INT64_TYPE__
#endif
#ifndef __UINT_FAST64_TYPE__
#define __UINT_FAST64_TYPE__ __UINT64_TYPE__
#endif

/* 7.18.1.4 Integer types capable of holding object pointers */

#ifndef _LP64
#ifndef __INTPTR_TYPE__
#define __INTPTR_TYPE__ __INT32_TYPE__
#endif
#ifndef __UINTPTR_TYPE__
#define __UINTPTR_TYPE__ __UINT32_TYPE__
#endif
#ifndef __PTRDIFF_TYPE__
#define __PTRDIFF_TYPE__ __INT32_TYPE__
#endif
#else
#ifndef __INTPTR_TYPE__
#define __INTPTR_TYPE__ __INT64_TYPE__
#endif
#ifndef __UINTPTR_TYPE__
#define __UINTPTR_TYPE__ __UINT64_TYPE__
#endif
#ifndef __PTRDIFF_TYPE__
#define __PTRDIFF_TYPE__ __INT64_TYPE__
#endif
#endif

/* 7.18.1.5 Greatest-width integer types */

#ifndef __INTMAX_TYPE__
#define __INTMAX_TYPE__ __INT64_TYPE__
#endif
#ifndef __UINTMAX_TYPE__
#define __UINTMAX_TYPE__ __UINT64_TYPE__
#endif

#endif /* !_COMMON_INT_MWGWTYPES_H_ */
