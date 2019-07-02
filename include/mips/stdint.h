#ifndef _MIPS_STDINT_H_
#define _MIPS_STDINT_H_

/*
 * 7.18.1 Integer types
 */

/* 7.18.1.1 Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int int16_t;
typedef unsigned short int uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int int64_t;
typedef unsigned long long int uint64_t;

/* 7.18.1.2 Minimum-width integer types */
typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;
typedef short int int_least16_t;
typedef unsigned short int uint_least16_t;
typedef int int_least32_t;
typedef unsigned int uint_least32_t;
typedef long long int int_least64_t;
typedef unsigned long long int uint_least64_t;

/* 7.18.1.3 Fastest minimum-width integer types */
typedef int int_fast8_t;
typedef unsigned int uint_fast8_t;
typedef int int_fast16_t;
typedef unsigned int uint_fast16_t;
typedef int int_fast32_t;
typedef unsigned int uint_fast32_t;
typedef long long int int_fast64_t;
typedef unsigned long long int uint_fast64_t;

/* 7.18.1.4 Integer types capable of holding object pointers */
typedef int intptr_t;
typedef unsigned int uintptr_t;

/* 7.18.1.5 Greatest-width integer types */
typedef long long int intmax_t;
typedef unsigned long long int uintmax_t;

#endif /* !_MIPS_STDINT_H_ */
