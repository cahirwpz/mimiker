#include <sys/mimiker.h>
#include <sys/vm.h>
#include <sys/thread.h>
#include <sys/klog.h>
#include <sys/cpu.h>
#include <sys/kcsan.h>

/*
 * When KCSAN is enabled, the code is instrumented to allow the monitoring of
 * memory accesses. Specifically, each memory access will be augmented by a
 * function call. The compiler distinguishes different memory accesses depending
 * on the type of the variable: plain, volatile or atomic. In our implementation
 * last two are discarded, because they don't meet the conditions of the
 * previously defined data race.
 *
 * On every memory access KCSAN, basically do two things: checks for the already
 * existing data race on that address and occasionally sets up a watchpoint. The
 * second part is skipped (SKIP_COUNT - 1) out of SKIP_COUNT times; to do
 * otherwise would slow the kernel to a point of complete unusability. On the
 * SKIP_COUNT th time, though, KCSAN keeps an eye on the address for a period of
 * time, looking for other accesses. While running in the context of the thread
 * where the access was performed, KCSAN will set a "watchpoint", which is done
 * by recording the address, the size of the data access, and whether the access
 * was a write in a small table. This thread will then simply delay by yielding
 * control to another thread.
 *
 * Note that, before deciding whether to ignore an access, KCSAN looks to see if
 * there is already a watchpoint established for the address. If so, and if
 * either the current access or the access that created the watchpoint is a
 * write, then a race condition has been detected and a report will be sent to
 * the system log.
 *
 * Meanwhile, the original thread is delaying after having set the watchpoint.
 * At the end of the delay period, the watchpoint will be deleted and monitoring
 * of that address stops. But before execution continues, the value at the
 * accessed address will be checked; if it has changed since the watchpoint was
 * set, a race condition is once again deemed to have occurred.
 */

/* If the architecture is 64 bit */
#ifdef _LP64
#define WATCHPOINT_READ_BIT (1L << 63)
#define WATCHPOINT_SIZE_SHIFT 61
#else /* _LP64 */
#define WATCHPOINT_READ_BIT (1L << 31)
#define WATCHPOINT_SIZE_SHIFT 29
#endif /* _LP64 */

#define WATCHPOINT_SIZE_MASK (~(WATCHPOINT_ADDR_MASK | WATCHPOINT_READ_BIT))
#define WATCHPOINT_ADDR_MASK ((1L << WATCHPOINT_SIZE_SHIFT) - 1)

#define WATCHPOINT_INVALID 0L

/*
 * These values were chosen rather arbitrarily, so feel free to modify them if
 * for example higher performance is needed.
 */
#define WATCHPOINT_NUM 32
#define SKIP_COUNT 500

#define MAX_ENCODABLE_SIZE 8

static atomic_long watchpoints[WATCHPOINT_NUM];

/* Stats */
static atomic_int kcsan_slot_taken_count;
static atomic_int kcsan_set_watchpoint_count;

/* How many memory accesses should be skipped before we create a watchpoint. */
static atomic_int skip_counter = SKIP_COUNT;

static int kcsan_ready;

static inline long encode_watchpoint(uintptr_t addr, size_t size,
                                     bool is_read) {
  return (is_read ? WATCHPOINT_READ_BIT : 0) |
         (((long)log2(size)) << WATCHPOINT_SIZE_SHIFT) |
         (addr & WATCHPOINT_ADDR_MASK);
}

static inline bool decode_watchpoint(long watchpoint, uintptr_t *addr,
                                     size_t *size, bool *is_read) {
  if (watchpoint == WATCHPOINT_INVALID)
    return false;

  *addr = watchpoint & WATCHPOINT_ADDR_MASK;
  *size = 1L << ((watchpoint & WATCHPOINT_SIZE_MASK) >> WATCHPOINT_SIZE_SHIFT);
  *is_read = (watchpoint & WATCHPOINT_READ_BIT) == WATCHPOINT_READ_BIT;
  return true;
}

static inline bool address_match(uintptr_t addr1, size_t size1, uintptr_t addr2,
                                 size_t size2) {
  addr1 &= WATCHPOINT_ADDR_MASK;
  addr2 &= WATCHPOINT_ADDR_MASK;
  return (addr1 < addr2 + size2) && (addr2 < addr1 + size1);
}

static inline int watchpoint_slot(uintptr_t addr) {
  return (addr / PAGESIZE) % WATCHPOINT_NUM;
}

/*
 * For a given memory access returns the conflicting watchpoint if the data race
 * is found. Otherwise returns NULL.
 */
static inline atomic_long *search_for_race(uintptr_t addr, size_t size,
                                           bool is_read) {
  uintptr_t other_addr;
  size_t other_size;
  bool other_is_read;

  int slot = watchpoint_slot(addr);
  atomic_long *watchpoint_p = &watchpoints[slot];

  long encoded = atomic_load(watchpoint_p);
  if (!decode_watchpoint(encoded, &other_addr, &other_size, &other_is_read))
    return NULL;

  /*
   * Data race occurs when there are two memory accesses and at least one of
   * them is write.
   */
  if (other_is_read && is_read)
    return NULL;

  if (address_match(addr, size, other_addr, other_size))
    return watchpoint_p;

  return NULL;
}

static inline atomic_long *insert_watchpoint(uintptr_t addr, size_t size,
                                             bool is_read) {
  int slot = watchpoint_slot(addr);
  atomic_long *watchpoint_p = &watchpoints[slot];

  long encoded = encode_watchpoint(addr, size, is_read);
  long expected = WATCHPOINT_INVALID;
  if (!atomic_compare_exchange_strong(watchpoint_p, &expected, encoded))
    return NULL;
  return watchpoint_p;
}

static inline uint64_t read_mem(uintptr_t addr, size_t size) {
  switch (size) {
    case 1:
      return *((uint8_t *)addr);
    case 2:
      return *((uint16_t *)addr);
    case 4:
      return *((uint32_t *)addr);
    case 8:
      return *((uint64_t *)addr);
    default:
      return 0;
  }
}

static inline void setup_watchpoint(uintptr_t addr, size_t size, bool is_read) {
  uint64_t prev_val = 0, new_val = 0;

  /* Create a watchpoint if the counter reaches 0. */
  if (atomic_fetch_sub(&skip_counter, 1) != 1)
    return;

  /* Reset the counter. */
  atomic_store(&skip_counter, SKIP_COUNT);

  atomic_long *watchpoint_p = insert_watchpoint(addr, size, is_read);
  if (watchpoint_p == NULL) {
    atomic_fetch_add(&kcsan_slot_taken_count, 1);
    return;
  }

  atomic_fetch_add(&kcsan_set_watchpoint_count, 1);

  prev_val = read_mem(addr, size);
  thread_yield();
  new_val = read_mem(addr, size);

  if (__predict_false(prev_val != new_val)) {
    panic("===========KernelConcurrencySanitizer===========\n"
          "* value of the watched variable %p has changed\n"
          "* %s of size %lu\n"
          "* previous value %x\n"
          "* current value %x\n"
          "============================================",
          (void *)addr, (is_read ? "read" : "write"), size, prev_val, new_val);
  }

  atomic_store(watchpoint_p, WATCHPOINT_INVALID);
}

static void kcsan_check(uintptr_t addr, size_t size, bool is_read) {
  if (!kcsan_ready || cpu_intr_disabled() || preempt_disabled())
    return;

  /* Not every instrumented memory address is in the kernel space. For an
   * example, initrd is outside of this range. */
  if (addr < KERNEL_SPACE_BEGIN)
    return;

  /* For the sake of simplicity, we consider only accesses of size
   * 1, 2, 4, 8. Accesses with other sizes can occur when for example structs
   * are copied with a assignment statement. */
  if (size > MAX_ENCODABLE_SIZE || !powerof2(size))
    return;

  atomic_long *watchpoint = search_for_race(addr, size, is_read);
  if (__predict_true(watchpoint == NULL)) {
    setup_watchpoint(addr, size, is_read);
  } else {
    panic("===========KernelConcurrencySanitizer===========\n"
          "* found data race on the variable %p\n"
          "* %s of size %lu\n"
          "* you can find the second thread using gdb\n"
          "============================================",
          (void *)addr, (is_read ? "read" : "write"), size);
  }
}

/* Instrumentation of plain memory accesses */
#define DEFINE_KCSAN_READ_WRITE(size)                                          \
  void __tsan_read##size(void *ptr) {                                          \
    kcsan_check((uintptr_t)ptr, size, true);                                   \
  }                                                                            \
  void __tsan_write##size(void *ptr) {                                         \
    kcsan_check((uintptr_t)ptr, size, false);                                  \
  }

DEFINE_KCSAN_READ_WRITE(1);
DEFINE_KCSAN_READ_WRITE(2);
DEFINE_KCSAN_READ_WRITE(4);
DEFINE_KCSAN_READ_WRITE(8);
DEFINE_KCSAN_READ_WRITE(16);

void __tsan_read_range(void *ptr, size_t size) {
  kcsan_check((uintptr_t)ptr, size, true);
}

void __tsan_write_range(void *ptr, size_t size) {
  kcsan_check((uintptr_t)ptr, size, false);
}

/*
 * We discard memory accesses on volatile variables, as they can be atomic on
 * some architectures. And because of that they don't fulfill the requirements
 * of a data race.
 */
#define DEFINE_KCSAN_VOLATILE_READ_WRITE(size)                                 \
  void __tsan_volatile_read##size(void *ptr) {                                 \
  }                                                                            \
  void __tsan_volatile_write##size(void *ptr) {                                \
  }

DEFINE_KCSAN_VOLATILE_READ_WRITE(1);
DEFINE_KCSAN_VOLATILE_READ_WRITE(2);
DEFINE_KCSAN_VOLATILE_READ_WRITE(4);
DEFINE_KCSAN_VOLATILE_READ_WRITE(8);
DEFINE_KCSAN_VOLATILE_READ_WRITE(16);

/* These functions aren't used KCSAN, but they have to be provied to the
 * compiler. */
void __tsan_func_entry(void *call_pc) {
}
void __tsan_func_exit(void) {
}
void __tsan_init(void) {
}

/*
 * The compiler replaces all atomic operations to these functions. KCSAN doesn't
 * use them, so we simply call the original ones.
 */
#define DEFINE_KCSAN_ATOMIC_OP(size, op)                                       \
  uint##size##_t __tsan_atomic##size##_##op(volatile uint##size##_t *a,        \
                                            uint##size##_t v, int mo) {        \
    return atomic_##op##_explicit(a, v, mo);                                   \
  }

#define DEFINE_KCSAN_ATOMIC_OPS(size)                                          \
  DEFINE_KCSAN_ATOMIC_OP(size, exchange)                                       \
  DEFINE_KCSAN_ATOMIC_OP(size, fetch_add)                                      \
  DEFINE_KCSAN_ATOMIC_OP(size, fetch_sub)                                      \
  DEFINE_KCSAN_ATOMIC_OP(size, fetch_or)                                       \
  uint##size##_t __tsan_atomic##size##_load(const uint##size##_t *a, int mo) { \
    return atomic_load_explicit(a, mo);                                        \
  }                                                                            \
  void __tsan_atomic##size##_store(volatile uint##size##_t *a,                 \
                                   volatile uint##size##_t v, int mo) {        \
    atomic_store_explicit(a, v, mo);                                           \
  }                                                                            \
  int __tsan_atomic##size##_compare_exchange_strong(                           \
    volatile uint##size##_t *a, uint##size##_t *c, uint##size##_t v, int mo,   \
    int fail_mo) {                                                             \
    return atomic_compare_exchange_strong_explicit(a, c, v, mo, fail_mo);      \
  }

DEFINE_KCSAN_ATOMIC_OPS(8);
DEFINE_KCSAN_ATOMIC_OPS(16);
DEFINE_KCSAN_ATOMIC_OPS(32);

#ifdef _LP64
DEFINE_KCSAN_ATOMIC_OPS(64);
#endif /* _LP64 */

void init_kcsan(void) {
  kcsan_ready = 1;
}
