#ifndef _STDDEF_H_
#define _STDDEF_H_

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef unsigned long size_t;
typedef __WCHAR_TYPE__ wchar_t;

/* A null pointer constant. */
#define NULL (void *)0

/* Offset of member MEMBER in a struct of type TYPE. */
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

/* Type whose alignment is supported in every context and is at least
 * as great as that of any standard type not using alignment specifiers. */
typedef union {
  void *_v;
  long double _ld;
  long long int _ll;
} max_align_t;

#endif /* !_STDDEF_H_ */
