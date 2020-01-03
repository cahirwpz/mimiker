#include <sys/mimiker.h>

int copystr(const void *restrict kfaddr, void *restrict kdaddr, size_t len,
            size_t *restrict lencopied) {
  panic("Not implemented!");
}

int copyinstr(const void *restrict udaddr, void *restrict kaddr, size_t len,
              size_t *restrict lencopied) {
  panic("Not implemented!");
}

int copyin(const void *restrict udaddr, void *restrict kaddr, size_t len) {
  panic("Not implemented!");
}

int copyout(const void *restrict kaddr, void *restrict udaddr, size_t len) {
  panic("Not implemented!");
}

bool try_load_word(unsigned *ptr, unsigned *val_p) {
  panic("Not implemented!");
}

bool try_store_word(unsigned *ptr, unsigned val) {
  panic("Not implemented!");
}
