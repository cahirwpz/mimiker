/* Linker scripts are documented here:
 * https://sourceware.org/binutils/docs/ld/Scripts.html */
OUTPUT_ARCH(mips)
ENTRY(_start)
PHDRS
{
  text   PT_LOAD FLAGS(5);      /* read-only, executable */
  rodata PT_LOAD FLAGS(4);      /* read-only */
  data   PT_LOAD FLAGS(6);      /* read-write */
}
PROVIDE(__tlb_stride_length = 1);
SECTIONS
{
  /* RAM is mapped in kseg0 (cacheable) and kseg1 (non-cacheable) */
  _ram = 0x80000000;

  .text 0x80100000: AT(0x00100000) ALIGN(4096)
  {
    __kernel_start = .;
    /* Exception handlers. */
    *(.ebase)
    . = ALIGN(4096);
    __text = ABSOLUTE(.);
    *(.text .text.*)
    __etext = ABSOLUTE(.);
  } : text

  /* The kernel expects the ramdisk start & end addresses
   * to be page-aligned. */
  .initrd : ALIGN(4096)
  {
    __rd_start = ABSOLUTE(.);
    *(.initrd)
    . = ALIGN(4096);
    __rd_end = ABSOLUTE(.);
  } : rodata /* The ramdisk is read-only, so place it in read-only segment. */

  /* In order to be addressable using a 16-bit signed offset from _gp,
   * this section must be placed close to the .data section, as
   * _gp is defined here as the start of the .data section.  */
  .rodata : ALIGN(4)
  {
     *(.rodata .rodata.*)
  } : rodata

  .data : ALIGN(4096)
  {
    __data = ABSOLUTE(.);
    _gp = .;
    *(.data .data.*)
    *(.sdata .sdata.*)
    *(.eh_frame .eh_frame.*)
    . = ALIGN (4);
    __edata = ABSOLUTE(.);
  } : data

  .bss : ALIGN(4)
  {
    __bss = ABSOLUTE(.);
    *(.sbss .scommon)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN (4);
    __ebss = ABSOLUTE(.);
    /* add one page for early malloc */
    . = . + 4096;
    . = ALIGN (4096);
  }

  __kernel_end = .;

  /* Sections to be discarded */
  /DISCARD/ :
  {
    *(.comment)
    *(.note)
    *(.options)
    *(.pdr)
    *(.reginfo)
    *(.gnu.attributes)
    *(.MIPS.abiflags)
    *(.MIPS.options)
  }
}
