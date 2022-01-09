OUTPUT_FORMAT("elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)
PHDRS
{
  boot PT_LOAD FLAGS(5);
  text PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data PT_LOAD FLAGS(6);
}
PROVIDE(__image_size = (__kernel_end - __kernel_start) + _boot_size);
SECTIONS
{
  .boot 0x40000000 : AT(0x40000000) ALIGN(4096)
  {
    __boot = ABSOLUTE(.);
    KEEP(riscv/riscv.ka:start.o)
    *(.boot .boot.*)
    . = ALIGN(4096);
    __eboot = ABSOLUTE(.);
  } : boot
  HIDDEN(_boot_size = __eboot - __boot);
  .text -2147483648 + _boot_size : AT(__eboot) ALIGN(4096)
  {
    __kernel_start = ABSOLUTE(.);
    __text = ABSOLUTE(.);
    *(.text .text.*)
    __etext = ABSOLUTE(.);
  } : text
  .rodata :
  {
    *(.rodata .rodata.*)
  } : rodata
  .data : ALIGN(4096)
  {
    __data = ABSOLUTE(.);
    *(.sdata .sdata.*)
    *(.data .data.*)
    __edata = ABSOLUTE(.);
  } : data
  .bss :
  {
    __bss = ABSOLUTE(.);
    *(.sbss .scommon)
    *(.bss .bss.*)
    *(COMMON)
    __ebss = ABSOLUTE(.);
    __kernel_end = ABSOLUTE(.);
  }
  HIDDEN(_gp_off = 4K / 2);
  __global_pointer$ = MIN(__data + _gp_off, __ebss - _gp_off);
  /DISCARD/ :
  {
    *(.comment)
    *(.gnu.attributes)
    *(.note .note.*)
    *(.options)
    *(.pdr)
    *(.reginfo)
  }
}
