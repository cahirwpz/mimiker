#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/vm_map.h>
#include <sys/ustack.h>
#include <machine/vm_param.h>
#include <machine/abi.h>

static inline bool finalized_p(ustack_t *us) {
  return us->us_finalized;
}

static inline bool enough_free_space_p(ustack_t *us, size_t len) {
  return us->us_bottom + len <= us->us_limit;
}

void ustack_setup(ustack_t *us, vaddr_t user_top, size_t capacity) {
  capacity = align(capacity, STACK_ALIGN);

  us->us_user_top = user_top;
  us->us_top = kmalloc(M_TEMP, capacity, 0);
  us->us_limit = us->us_top + capacity;
  us->us_bottom = us->us_top;
  us->us_finalized = false;
}

void ustack_teardown(ustack_t *us) {
  kfree(M_TEMP, us->us_top);
}

static int ustack_align(ustack_t *us, size_t howmuch) {
  void *new_bottom = align(us->us_bottom, howmuch);
  if (new_bottom > us->us_limit)
    return ENOMEM;
  us->us_bottom = new_bottom;
  return 0;
}

static int ustack_alloc_aligned(ustack_t *us, void **ptr, size_t len,
                                size_t alignment) {
  assert(!finalized_p(us));

  int error;
  if ((error = ustack_align(us, alignment)))
    return error;
  if (!enough_free_space_p(us, len))
    return ENOMEM;
  *ptr = us->us_bottom;
  us->us_bottom += len;
  return 0;
}

#define ustack_alloc(us, ptr, len, size)                                       \
  ustack_alloc_aligned((us), (void **)(ptr), (len), (size))

int ustack_alloc_string(ustack_t *us, size_t len, char **str_p) {
  return ustack_alloc(us, str_p, len + 1, sizeof(char));
}

int ustack_alloc_ptr_n(ustack_t *us, size_t count, vaddr_t *kva_p) {
  return ustack_alloc(us, kva_p, sizeof(void *) * count, sizeof(void *));
}

#define DEFINE_USTACK_PUSH(TYPE)                                               \
  int ustack_push_##TYPE(ustack_t *us, TYPE value) {                           \
    int error;                                                                 \
    TYPE *value_p;                                                             \
    if ((error = ustack_alloc(us, &value_p, sizeof(TYPE), sizeof(TYPE))))      \
      return error;                                                            \
    *value_p = value;                                                          \
    return 0;                                                                  \
  }

DEFINE_USTACK_PUSH(int);
DEFINE_USTACK_PUSH(long);

void ustack_relocate_ptr(ustack_t *us, vaddr_t *ptr_p) {
  assert(finalized_p(us));
  void *ptr = (void *)*ptr_p;
  assert(ptr <= us->us_bottom && ptr >= us->us_top);
  *ptr_p = us->us_user_top - (us->us_bottom - ptr);
}

void ustack_finalize(ustack_t *us) {
  assert(!finalized_p(us));
  /* Cannot fail because initially stack is aligned to STACK_ALIGN. */
  ustack_align(us, STACK_ALIGN);
  us->us_finalized = true;
}

int ustack_copy(ustack_t *us, vaddr_t *new_user_top_p) {
  size_t size = us->us_bottom - us->us_top;
  vaddr_t user_top = us->us_user_top - size;
  *new_user_top_p = user_top;
  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, (void *)user_top, size);
  return uiomove(us->us_top, size, &uio);
}
