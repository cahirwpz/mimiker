#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <sys/cdefs.h>
#include <machine/setjmp.h>

typedef _jmpbuf_slot_t sigjmp_buf[_JMPBUF_LEN + 1];
typedef _jmpbuf_slot_t jmp_buf[_JMPBUF_LEN];

__BEGIN_DECLS
int setjmp(jmp_buf) __returns_twice;
void longjmp(jmp_buf, int) __noreturn;
int sigsetjmp(sigjmp_buf, int) __returns_twice;
void siglongjmp(sigjmp_buf, int) __noreturn;
__END_DECLS

#endif /* !_SETJMP_H_ */
