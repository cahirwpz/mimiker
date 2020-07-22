#ifndef _AARCH64_PCPU_H_
#define _AARCH64_PCPU_H_

#define PCPU_MD_FIELDS                                                         \
  struct {}

#ifdef _MACHDEP
#ifdef __ASSEMBLER__

#define LOAD_PCPU(reg) LA reg, _pcpu_data

#endif /* !__ASSEMBLER__ */
#endif /* !_MACHDEP */

#endif /* !_AARCH64_PCPU_H_ */
