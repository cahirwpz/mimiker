#include <stdc.h>
#include <malloc.h>
#include <errno.h>
#include <uio.h>
#include <vm_map.h>
#include <mips/ustack.h>

#define STACK_ALIGNMENT 8 /* According to MIPS SystemV ABI */

#define ADDR_IN_RANGE(left, addr, right)                                       \
  ((((void *)(left)) <= ((void *)(addr))) &&                                   \
   (((void *)(addr)) <= ((void *)(right))))

static inline bool finalized_p(ustack_t *us) {
  return us->us_finalized;
}

static inline bool enough_free_space_p(ustack_t *us, size_t len) {
  return us->us_bottom + len <= us->us_limit;
}

void ustack_setup(ustack_t *us, vaddr_t user_top, size_t capacity) {
  capacity = align(capacity, STACK_ALIGNMENT);

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
    return -ENOMEM;
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
    return -ENOMEM;
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

int ustack_push_int(ustack_t *us, int value) {
  int error;
  int *value_p;
  if ((error = ustack_alloc(us, &value_p, sizeof(int), sizeof(int))))
    return error;
  *value_p = value;
  return 0;
}

int ustack_store_strings(ustack_t *us, const char **str_p, char **stack_str_p,
                         size_t howmany) {
  if (howmany > 0) {
    assert(ADDR_IN_RANGE(us->us_top, stack_str_p, us->us_limit));
    assert(ADDR_IN_RANGE(us->us_top, stack_str_p + howmany - 1, us->us_limit));
  }

  int error = 0;
  /* Store arguments, creating the argument vector. */
  for (size_t i = 0; i < howmany; i++) {
    size_t n = strlen(str_p[i]);
    if ((error = ustack_alloc_string(us, n, &stack_str_p[i])))
      goto fail;
    memcpy(stack_str_p[i], str_p[i], n + 1);
  }

fail:
  return error;
}

void ustack_store_nullptr(ustack_t *us, char **where) {
  assert(ADDR_IN_RANGE(us->us_top, where, us->us_limit));

  *where = NULL;
}

void ustack_relocate_ptr(ustack_t *us, vaddr_t *ptr_p) {
  assert(finalized_p(us));
  void *ptr = (void *)*ptr_p;
  assert(ptr <= us->us_bottom && ptr >= us->us_top);
  *ptr_p = us->us_user_top - (us->us_bottom - ptr);
}

void ustack_finalize(ustack_t *us) {
  assert(!finalized_p(us));
  /* Cannot fail because initially stack is aligned to STACK_ALIGNMENT. */
  ustack_align(us, STACK_ALIGNMENT);
  us->us_finalized = true;
}

int ustack_copy(ustack_t *us, vaddr_t *new_user_top_p) {
  size_t size = us->us_bottom - us->us_top;
  vaddr_t user_top = us->us_user_top - size;
  *new_user_top_p = user_top;
  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, (void *)user_top, size);
  return uiomove(us->us_top, size, &uio);
}
