#include <sys/mimiker.h>
#include <sys/vm.h>
#include <machine/interrupt.h>

#define WATCHPOINT_READ_BIT (1 << 31)
#define WATCHPOINT_SIZE_SHIFT 28
#define WATCHPOINT_SIZE_MASK (~(WATCHPOINT_ADDR_MASK | WATCHPOINT_READ_BIT))
#define WATCHPOINT_ADDR_MASK ((1 << WATCHPOINT_SIZE_SHIFT) - 1)

#define WATCHPOINT_INVALID 0

#define WATCHPOINT_NUM 32
#define SKIP_COUNT 500
#define WATCHPOINT_DELAY 10000

static atomic_int watchpoints[WATCHPOINT_NUM];

static atomic_int kcsan_setup_watchpoint_counter;

/* How many accesses should be skipped before we setup a watchpoint. */
static atomic_int skip_counter = SKIP_COUNT;

static inline int encode_watchpoint(uintptr_t addr, size_t size, bool is_read) {
  return (is_read ? WATCHPOINT_READ_BIT : 0) | (size << WATCHPOINT_SIZE_SHIFT) |
         (addr & WATCHPOINT_ADDR_MASK);
}

static inline bool decode_watchpoint(int watchpoint, uintptr_t *addr,
                                     size_t *size, bool *is_read) {
  if (watchpoint == WATCHPOINT_INVALID)
    return false;

  *addr = watchpoint & WATCHPOINT_ADDR_MASK;
  *size = (watchpoint & WATCHPOINT_SIZE_MASK) >> WATCHPOINT_SIZE_SHIFT;
  *is_read = (watchpoint & WATCHPOINT_READ_BIT) == WATCHPOINT_READ_BIT;
  return true;
}

static inline bool address_match(uintptr_t addr1, size_t size1, uintptr_t addr2,
                                 size_t size2) {
  return (addr1 <= addr2 + size2 - 1) && (addr2 <= addr1 + size1 - 1);
}

static inline int watchpoint_slot(uintptr_t addr) {
  return (addr / PAGESIZE) % WATCHPOINT_NUM;
}

static inline bool should_watch(void) {
  return atomic_fetch_sub(&skip_counter, 1) == 1;
}

static void delay(int count) {
  for (int i = 0; i < count; i++) {
    __asm__ volatile("nop");
  }
}

static inline atomic_int *find_watchpoint(uintptr_t addr, size_t size, bool is_read) {
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

static inline atomic_int *insert_watchpoint(uintptr_t addr, size_t size, bool is_read) {
  int slot = watchpoint_slot(addr);
  atomic_int *watchpoint_p = &watchpoints[slot];

  int encoded = encode_watchpoint(addr, size, is_read);
  int expected = WATCHPOINT_INVALID;
  if (!atomic_compare_exchange_strong(watchpoint_p, &expected, encoded))
    return NULL;
  return watchpoint_p;
}

static inline void setup_watchpoint(uintptr_t addr, size_t size, bool is_read) {
  	union {
      uint8_t b1;
      uint16_t b2;
      uint32_t b4;
      uint64_t b8;
	} value = {};

  /* Reset the counter. */
  atomic_store(&skip_counter, SKIP_COUNT);

  atomic_int *watchpoint_p = insert_watchpoint(addr, size, is_read);
  if (watchpoint_p == NULL)
    return;

  atomic_fetch_add(&kcsan_setup_watchpoint_counter, 1);

  switch (size) {
    case 1:
      value.b1 = *((uint8_t *)addr);
      break;
    case 2:
      value.b2 = *((uint16_t *)addr);
      break;
    case 4:
      value.b4 = *((uint32_t *)addr);
      break;
    case 8:
      value.b8 = *((uint64_t *)addr);
      break;
    default:
      break;
  }

  delay(WATCHPOINT_DELAY);
  
  switch (size) {
    case 1:
      value.b1 ^= *((uint8_t *)addr);
      break;
    case 2:
      value.b2 ^= *((uint16_t *)addr);
      break;
    case 4:
      value.b4 ^= *((uint32_t *)addr);
      break;
    case 8:
      value.b8 ^= *((uint64_t *)addr);
      break;
    default:
      break;
  }

  if (value.b8 != 0) {
    panic("The value of the variable has changed!");
  }

  atomic_store(watchpoint_p, WATCHPOINT_INVALID);
}

static void kcsan_check(uintptr_t addr, size_t size, bool is_read) {
  if (cpu_intr_disabled())
    return;

  atomic_int *watchpoint = find_watchpoint(addr, size, is_read);
  if (watchpoint == NULL) {
    if (should_watch())
      setup_watchpoint(addr, size, is_read);
  } else {
    panic("Found data race");
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

void __tsan_func_entry(void *call_pc) {
}
void __tsan_func_exit(void) {
}
void __tsan_init(void) {
}

typedef char __tsan_atomic8;
typedef short __tsan_atomic16;
typedef int __tsan_atomic32;
typedef long __tsan_atomic64;

typedef enum {
  __tsan_memory_order_relaxed,
  __tsan_memory_order_consume,
  __tsan_memory_order_acquire,
  __tsan_memory_order_release,
  __tsan_memory_order_acq_rel,
  __tsan_memory_order_seq_cst
} __tsan_memory_order;

__tsan_atomic32 __tsan_atomic32_load(const __tsan_atomic32 *a,
                                     __tsan_memory_order mo) {
  return atomic_load_explicit(a, mo);
}

void __tsan_atomic32_store(volatile __tsan_atomic32 *a,
                           volatile __tsan_atomic32 v, __tsan_memory_order mo) {
  atomic_store_explicit(a, v, mo);
}

__tsan_atomic32 __tsan_atomic32_exchange(volatile __tsan_atomic32 *a,
                                         __tsan_atomic32 v,
                                         __tsan_memory_order mo) {
  return atomic_exchange_explicit(a, v, mo);
}

__tsan_atomic32 __tsan_atomic32_fetch_add(volatile __tsan_atomic32 *a,
                                          __tsan_atomic32 v,
                                          __tsan_memory_order mo) {
  return atomic_fetch_add_explicit(a, v, mo);
}

__tsan_atomic32 __tsan_atomic32_fetch_sub(volatile __tsan_atomic32 *a,
                                          __tsan_atomic32 v,
                                          __tsan_memory_order mo) {
  return atomic_fetch_sub_explicit(a, v, mo);
}

__tsan_atomic32 __tsan_atomic32_fetch_or(volatile __tsan_atomic32 *a,
                                         __tsan_atomic32 v,
                                         __tsan_memory_order mo) {
  return atomic_fetch_or_explicit(a, v, mo);
}

int __tsan_atomic32_compare_exchange_strong(volatile __tsan_atomic32 *a,
                                            __tsan_atomic32 *c,
                                            __tsan_atomic32 v,
                                            __tsan_memory_order mo,
                                            __tsan_memory_order fail_mo) {
  return atomic_compare_exchange_strong_explicit(a, c, v, mo, fail_mo);
}