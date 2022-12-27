#ifndef _SYS_LINKER_SET_H_
#define _SYS_LINKER_SET_H_

#include <sys/cdefs.h>

/* The implementation mostly follows DragonFly BSD's 'sys/linker_set.h'
 *
 * Having declared a linker set, you can assign items to the set in various
 * compilation units. Linker will go over all compilation units and gather all
 * items belonging to the set into an array of pointers to the items.
 * These arrays are often referred to as linker sets.
 *
 * The order of items in a linker set is implementation specific. Only items
 * linked into final file will be placed in corresponding linker set. */

/* GLOBL macro exists to preserve __start_set_* and __stop_set_* sections of
 * kernel modules which are discarded from binutils 2.17.50+ otherwise. */
#define __GLOBL1(sym) __asm__(".globl " #sym)
#define __GLOBL(sym) __GLOBL1(sym)

/* Add \sym entry to the linker set where \sym type must be same as \ptype
 * of the linker set */
#define SET_ENTRY(set, sym)                                                    \
  __GLOBL(__CONCAT(__start_set_, set));                                        \
  __GLOBL(__CONCAT(__stop_set_, set));                                         \
  static void const *const __set_##set##_sym_##sym __section("set_" #set)      \
    __used = &sym

/* Define a new linker set with \set name and entries of \ptype type */
#define SET_DECLARE(set, ptype)                                                \
  extern ptype *__CONCAT(__start_set_, set);                                   \
  extern ptype *__CONCAT(__stop_set_, set)

/* Returns the begining of the linker set of type **T
 * where T is \ptype type of the linker set */
#define SET_BEGIN(set) (&__CONCAT(__start_set_, set))

/* Returns the end of the linker set of type **T
 * where T is \ptype type of the linker set */
#define SET_LIMIT(set) (&__CONCAT(__stop_set_, set))

/* Iterate over all the elements of a set. Sets always contain addresses of
 * items, and "pvar" points to words containing those addresses. Thus is must
 * be declared as "type **pvar", and the address of each set item is obtained
 * inside the loop by "*pvar". */
#define SET_FOREACH(pvar, set)                                                 \
  for (pvar = SET_BEGIN(set); pvar < SET_LIMIT(set); pvar++)

/* Gets the ith item from the set. */
#define SET_ITEM(set, i) ((SET_BEGIN(set))[i])

/* Provides a count of the items in a set. */
#define SET_COUNT(set) (SET_LIMIT(set) - SET_BEGIN(set))

#define INVOKE_CTORS(set)                                                      \
  {                                                                            \
    typedef void ctor_t(void);                                                 \
    SET_DECLARE(set, ctor_t);                                                  \
    ctor_t **ctor_p;                                                           \
    SET_FOREACH (ctor_p, set)                                                  \
      (*ctor_p)();                                                             \
  }

#endif /* !_SYS_LINKER_SET_H_ */
