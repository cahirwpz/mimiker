#include <sys/cdefs.h>

#undef assert

#ifdef NDEBUG
#define assert(e) (__static_cast(void, 0))
#else
#define assert(e)                                                              \
  ((e) ? __static_cast(void, 0) : __assert(__FILE__, __LINE__, __func__, #e))
#endif /* NDEBUG */

#ifndef _DIAGNOSTIC
#define _DIAGASSERT(e) (__static_cast(void, 0))
#else
#define _DIAGASSERT(e)                                                         \
  ((e) ? __static_cast(void, 0)                                                \
       : __diagassert(__FILE__, __LINE__, __func__, #e))
#endif /* _DIAGNOSTIC */

#ifndef __ASSERT_DECLARED
#define __ASSERT_DECLARED
__BEGIN_DECLS
__noreturn void __assert(const char *, int, const char *, const char *);
void __diagassert(const char *, int, const char *, const char *);
__END_DECLS
#endif /* __ASSERT_DECLARED */
