#ifndef _AARCH64_PCPU_H_
#define _AARCH64_PCPU_H_

#define PCPU_MD_FIELDS                                                         \
  struct {}

#ifdef _MACHDEP
#ifdef __ASSEMBLER__

/* clang-format off */
.macro  LOAD_PCPU reg
        ldr     \reg, _pcpu_data
.endm
/* clang-format on */

#endif /* !__ASSEMBLER__ */
#endif /* !_MACHDEP */

#endif /* !_AARCH64_PCPU_H_ */
