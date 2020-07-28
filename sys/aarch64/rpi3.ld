/* Linker scripts are documented here:
 * https://sourceware.org/binutils/docs/ld/Scripts.html */
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64) 
ENTRY(_start)

PHDRS
{
  boot   PT_LOAD FLAGS(5); /* read-only, executable */
  text   PT_LOAD FLAGS(5); /* read-only, executable */
  rodata PT_LOAD FLAGS(4); /* read-only */
  data   PT_LOAD FLAGS(6); /* read-write */
}

PROVIDE(_kernel = 0xffffffff00000000);

SECTIONS
{
  .boot 0x80000 : AT(0x80000) ALIGN(4096)
  {
    __boot = ABSOLUTE(.);
    KEEP(*(.boot))
    . = ALIGN(4096);
    *(.boot .boot.*)
    __eboot = ABSOLUTE(.);
  } : boot

  .text _kernel + __eboot : AT(__eboot) ALIGN(4096)
  {
    __kernel_start = ABSOLUTE(.);
    *(.evec);
    . = ALIGN(2048);
    __text = ABSOLUTE(.);
    *(.text .text.*)
    __etext = ABSOLUTE(.);
  } : text

  .rodata : ALIGN(4096)
  {
    *(.rodata .rodata.*)
  } : rodata

  .data : ALIGN(4096)
  {
    __data = ABSOLUTE(.);
    *(.data .data.*)
    . = ALIGN (4);
    __edata = ABSOLUTE(.);
  } : data

  .bss : ALIGN(4096)
  {
    __bss = ABSOLUTE(.);
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN (4);
    __ebss = ABSOLUTE(.);
    __kernel_end = ABSOLUTE(.);
  }

  /* Sections to be discarded */
  /DISCARD/ :
  {
    *(.ARM.*)
    *(.comment)
    *(.note)
    *(.options)
    *(.pdr)
    *(.reginfo)
    *(.gnu.attributes)
  }
}
