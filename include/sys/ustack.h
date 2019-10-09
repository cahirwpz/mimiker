#ifndef _SYS_USTACK_H_
#define _SYS_USTACK_H_

#include <sys/cdefs.h>
#include <machine/ustack.h>

/*! \file machine/ustack.h
 * \brief Machine dependent user-space stack frames construction.
 *
 * Stack contents is constructed from top to bottom manner.
 */

typedef struct ustack {
  vaddr_t us_user_top; /* UVA of stack top */
  void *us_limit;      /* the highest KVA of stack bottom (constant) */
  void *us_bottom;     /* KVA of stack bottom (grows upwards) */
  void *us_top;        /* KVA of stack top (constant) */
  bool us_finalized;   /* stack is properly aligned and cannot grow anymore */
} ustack_t;

/*! \brief Initializes passed ustack for use.
 *
 * \param{user_top} Constructed stack will be placed below this address.
 * \param{capacity} Maximum number of bytes in working stack.
 */
void ustack_setup(ustack_t *us, vaddr_t user_top, size_t capacity);

/*! \brief Releases all resources used by ustack. */
void ustack_teardown(ustack_t *us);

/*! \brief Allocates string on ustack.
 *
 * \param{str_p} Stores start address of allocated string.
 * \return ENOMEM if there was not enough space on ustack */
int ustack_alloc_string(ustack_t *us, size_t len, char **str_p);

/*! \brief Allocates an array of pointers on ustack.
 *
 * \param{kva_p} Stores start address of allocated pointer array.
 * \return ENOMEM if there was not enough space on ustack */
int ustack_alloc_ptr_n(ustack_t *us, size_t count, vaddr_t *kva_p);

/*! \brief Allocates single pointer on ustack. */
#define ustack_alloc_ptr(us, kva_p) ustack_alloc_ptr_n(us, 1, kva_p)

/*! \brief Pushes an integer on ustack.
 *
 * \return ENOMEM if there was not enough space on ustack */
int ustack_push_int(ustack_t *us, int value);

/*! \brief Pushes a long int on ustack.
 *
 * \return ENOMEM if there was not enough space on ustack */
int ustack_push_long(ustack_t *us, long value);

/*! \brief Fix size of ustack.
 *
 * Use when you pushed all contents onto the stack. */
void ustack_finalize(ustack_t *us);

/*! \brief Turn a kernel-space pointer into user-space pointer.
 *
 * This is useful if you pushed some pointers on ustack that refer to other data
 * on stack (e.g. pointers to strings). Those pointer must be relocated to
 * user-space and this routine helps with that.
 *
 * \note Assumes that stack will not grow anymore (is finalized).
 *
 * \param{ptr_p} Stores address to relocate.
 *               The address must be between `us_bottom` and `us_top`. */
void ustack_relocate_ptr(ustack_t *us, vaddr_t *ptr_p);

/*! \brief Copies constructed frames to user-space stack.
 *
 * \param{user_top_p} Stores new address of user-stack top. */
int ustack_copy(ustack_t *us, vaddr_t *new_user_top_p);

#endif /* !_SYS_USTACK_H_ */
