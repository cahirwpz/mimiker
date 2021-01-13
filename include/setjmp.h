#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <sys/cdefs.h>
#include <ucontext.h>

typedef ucontext_t sigjmp_buf[1];
typedef ucontext_t jmp_buf[1];

__BEGIN_DECLS
int setjmp(jmp_buf) __returns_twice;
void longjmp(jmp_buf, int) __noreturn;
int _setjmp(jmp_buf) __returns_twice;
void _longjmp(jmp_buf, int) __noreturn;
int sigsetjmp(sigjmp_buf, int) __returns_twice;
void siglongjmp(sigjmp_buf, int) __noreturn;

#ifdef _LIBC
void longjmperror(void);
#endif
__END_DECLS

#endif /* !_SETJMP_H_ */
