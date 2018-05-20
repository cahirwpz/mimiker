#ifndef _SYS_LINKER_SET_H_
#define _SYS_LINKER_SET_H_

#include <common.h>

/* The implementation mostly follows DragonFly BSD's 'sys/linker_set.h' */

/* GLOBL macro exists to preserve __start_set_* and __stop_set_* sections of
 * kernel modules which are discarded from binutils 2.17.50+ otherwise. */
#define __GLOBL1(sym) __asm__(".globl " #sym)
#define __GLOBL(sym) __GLOBL1(sym)

#define SET_ENTRY(set, sym)                                                    \
  __GLOBL(__CONCAT(__start_set_, set));                                        \
  __GLOBL(__CONCAT(__stop_set_, set));                                         \
  static void const *const __set_##set##_sym_##sym __section("set_" #set)      \
    __used = &sym

#define SET_DECLARE(set, ptype)                                                \
  extern ptype *__CONCAT(__start_set_, set);                                   \
  extern ptype *__CONCAT(__stop_set_, set)

#define SET_BEGIN(set) (&__CONCAT(__start_set_, set))
#define SET_LIMIT(set) (&__CONCAT(__stop_set_, set))

/* Iterate over all the elements of a set.  Sets always contain addresses of
 * things, and "pvar" points to words containing those addresses.  Thus is must
 * be declared as "type **pvar", and the address of each set item is obtained
 * inside the loop by "*pvar". */
#define SET_FOREACH(pvar, set)                                                 \
  for (pvar = SET_BEGIN(set); pvar < SET_LIMIT(set); pvar++)

/* Gets the ith item from the set. */
#define SET_ITEM(set, i) ((SET_BEGIN(set))[i])

/* Provides a count of the items in a set. */
#define SET_COUNT(set) (SET_LIMIT(set) - SET_BEGIN(set))

#endif /* !_SYS_LINKER_SET_H_ */
