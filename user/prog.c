#include <stdint.h>
#include <stdio.h>

#define BYTES(x) (sizeof(x)/sizeof(char))

#define STRING "This is an example string."
#define STRING_SIZE (BYTES(STRING))
#define TEXTAREA_SIZE ((STRING_SIZE - 1) * 3 + 1)
// This should land in .rodata, accessed by a pointer in .data
const char* string = STRING;
// This should land in .bss, accessed by a pointer in .data
char textarea[TEXTAREA_SIZE];

char arguments_received[1000];

uint32_t leaf(){
    return 0xdad0face;
}

void stack_test_func(uint32_t* arg){
    *arg = leaf();
}

void marquee(){
    uint32_t o = 0;
    while(1){
        o++;
        // Copy string three times with changing offsets
        for(int i = 0; i < TEXTAREA_SIZE - 1; i++){
            textarea[i] = string[(i + o) % (STRING_SIZE - 1)];
        }
        // Null-terminate
        textarea[TEXTAREA_SIZE - 1] = 0;
    }
}

int main(int argc, char** argv){
    // As currently there is no feasible way of outputting text, break
    // with debugger here to see argc/argv!

    uint32_t stack_variable = 0x42424242;
    stack_test_func(&stack_variable);
    marquee();
    return 0;
}
