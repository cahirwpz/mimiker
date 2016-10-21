#include <common.h>
#include <stdc.h>

extern uint8_t _binary_prog_uelf_start[];
extern uint8_t _binary_prog_uelf_size[];
extern uint8_t _binary_prog_uelf_end[];

void exec(){
    size_t prog_size = (size_t)(void*)_binary_prog_uelf_size;
    kprintf("[exec] User ELF size: %ld\n", prog_size);

    const uint8_t header_size = 64;
    const uint8_t stride = 8;
    kprintf("[exec] User ELF header dump:\n");
    for(int8_t i = 0; i < header_size; i++){
        if(i % stride == 0) kprintf("[exec]  ");
        uint8_t byte = _binary_prog_uelf_start[i];
        kprintf(" %02x", byte);
        if(i % stride == 7) kprintf("\n");
    }
}
