#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pool_alloc.h"

void int_constr(void *buf, __attribute__((unused)) size_t size) {
    int *num = buf;
    *num = 0;
}
void int_destr(__attribute__((unused)) void *buf, __attribute__((unused)) size_t size) {
    
}

typedef struct test {
    int foo;
    char bar[200];
} test_t;

void test_constr(void *buf, __attribute__((unused)) size_t size) {
    test_t *test = buf;
    test -> foo = 0;
    strcpy(test -> bar, "Hello, World!");
}

void test_destr(__attribute__((unused)) void *buf, __attribute__((unused)) size_t size) {
    
}

int main() {
    pool_t test;
    pool_init(&test, sizeof(int), int_constr, int_destr);
    int* item[10000];
    for (int i = 0; i < 10000; i++) {
        item[i] = pool_alloc(&test, 0); 
    }
    for (int i = 0; i < 10000; i++) {
        pool_free(&test, item[i]);
    }
    pool_destroy(&test);
    pool_t another_test;
    pool_init(&another_test, sizeof(test_t), test_constr, test_destr);
    test_t *another_item[10000];
    for (int i = 0; i < 10000; i++) {
        another_item[i] = pool_alloc(&another_test, 0);
    }
    //memset(another_item[0], 0, 100); //memory corruption test
    for (int i = 0; i < 10000; i++) {
        pool_free(&another_test, another_item[i]);
    }
    pool_destroy(&another_test);
}