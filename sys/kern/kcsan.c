#include <sys/mimiker.h>
#include <sys/vm.h>
#include <sys/thread.h>
#include <machine/interrupt.h>
#include <sys/klog.h>
#include <sys/kcsan.h>

#define WATCHPOINT_READ_BIT (1 << 31)
#define WATCHPOINT_SIZE_SHIFT 29
#define WATCHPOINT_SIZE_MASK (~(WATCHPOINT_ADDR_MASK | WATCHPOINT_READ_BIT))
#define WATCHPOINT_ADDR_MASK ((1 << WATCHPOINT_SIZE_SHIFT) - 1)

#define WATCHPOINT_INVALID 0

/*
 * These values were chosen rather arbitrarily, so feel free to modify them if
 * for example higher performance is needed.
 */
#define WATCHPOINT_NUM 32
#define SKIP_COUNT 500
#define WATCHPOINT_DELAY 10000

#define MAX_ENCODABLE_SIZE 8

static atomic_int watchpoints[WATCHPOINT_NUM];

/* Stats */
static atomic_int kcsan_slot_taken_count;
static atomic_int kcsan_set_watchpoint_count;

/* How many accesses should be skipped before we setup a watchpoint. */
static atomic_int skip_counter = SKIP_COUNT;

static int kcsan_ready;

static inline int encode_watchpoint(uintptr_t addr, size_t size, bool is_read) {
  return (is_read ? WATCHPOINT_READ_BIT : 0) |
         (log2(size) << WATCHPOINT_SIZE_SHIFT) | (addr & WATCHPOINT_ADDR_MASK);
}

static inline bool decode_watchpoint(int watchpoint, uintptr_t *addr,
                                     size_t *size, bool *is_read) {
  if (watchpoint == WATCHPOINT_INVALID)
    return false;

  *addr = watchpoint & WATCHPOINT_ADDR_MASK;
  *size = 1 << ((watchpoint & WATCHPOINT_SIZE_MASK) >> WATCHPOINT_SIZE_SHIFT);
  *is_read = (watchpoint & WATCHPOINT_READ_BIT) == WATCHPOINT_READ_BIT;
  return true;
}

static inline bool address_match(uintptr_t addr1, size_t size1, uintptr_t addr2,
                                 size_t size2) {
  return (addr1 < addr2 + size2) && (addr2 < addr1 + size1);
}

static inline int watchpoint_slot(uintptr_t addr) {
  return (addr / PAGESIZE) % WATCHPOINT_NUM;
}

static inline bool should_watch(void) {
  return atomic_fetch_sub(&skip_counter, 1) == 1;
}

static inline void delay(int count) {
  for (int i = 0; i < count; i++) {
    __asm__ volatile("nop");
  }
}

/*
 * For a given memory access returns the conflicting watchpoint if the data race
 * is found. Otherwise returns NULL.
 */
static inline atomic_int *search_for_race(uintptr_t addr, size_t size,
                                          bool is_read) {
  uintptr_t other_addr;
  size_t other_size;
  bool other_is_read;

  int slot = watchpoint_slot(addr);
  atomic_int *watchpoint_p = &watchpoints[slot];

  int encoded = atomic_load(watchpoint_p);
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

static inline atomic_int *insert_watchpoint(uintptr_t addr, size_t size,
                                            bool is_read) {
  int slot = watchpoint_slot(addr);
  atomic_int *watchpoint_p = &watchpoints[slot];

  int encoded = encode_watchpoint(addr, size, is_read);
  int expected = WATCHPOINT_INVALID;
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

  /* Reset the counter. */
  atomic_store(&skip_counter, SKIP_COUNT);

  atomic_int *watchpoint_p = insert_watchpoint(addr, size, is_read);
  if (watchpoint_p == NULL) {
    atomic_fetch_add(&kcsan_slot_taken_count, 1);
    return;
  }

  atomic_fetch_add(&kcsan_set_watchpoint_count, 1);

  prev_val = read_mem(addr, size);
  delay(WATCHPOINT_DELAY);
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

  /* For the sake of simplicity, we consider only accesses of size
   * 1, 2, 4, 8. */
  if (size > MAX_ENCODABLE_SIZE || !powerof2(size))
    return;

  if (addr < KERNEL_SPACE_BEGIN)
    return;

  atomic_int *watchpoint = search_for_race(addr, size, is_read);
  if (__predict_true(watchpoint == NULL)) {
    if (should_watch())
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

void __tsan_func_entry(void *call_pc) {
}
void __tsan_func_exit(void) {
}
void __tsan_init(void) {
}

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

void init_kcsan(void) {
  kcsan_ready = 1;
}
