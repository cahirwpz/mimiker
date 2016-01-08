#ifndef __ASM_H__
#define __ASM_H__

#define LEAF(name)\
    .##text;\
    .##globl    name;\
    .##ent  name;\
name:

#define END(name)\
    .##size name,.-name;\
    .##end  name

#endif
