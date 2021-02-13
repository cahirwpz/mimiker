#ifndef _LIMITS_H_
#define _LIMITS_H_

#include <sys/syslimits.h>

#define _POSIX2_BC_BASE_MAX 99
#define _POSIX2_BC_DIM_MAX 2048
#define _POSIX2_BC_SCALE_MAX 99
#define _POSIX2_BC_STRING_MAX 1000
#define _POSIX2_CHARCLASS_NAME_MAX 14
#define _POSIX2_COLL_WEIGHTS_MAX 2
#define _POSIX2_EQUIV_CLASS_MAX 2
#define _POSIX2_EXPR_NEST_MAX 32
#define _POSIX2_LINE_MAX 2048
#define _POSIX2_RE_DUP_MAX 255

/*
 * X/Open CAE Specifications,
 * adopted in IEEE Std 1003.1-2001 XSI.
 */
#define NL_TEXTMAX 2048

#define MB_LEN_MAX 32 /* Allow ISO/IEC 2022 */

#include <machine/limits.h>

#endif /* !_LIMITS_H_ */
