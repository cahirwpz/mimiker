/* Linker scripts are documented here:
 * https://sourceware.org/binutils/docs/ld/Scripts.html */
OUTPUT_ARCH(mips)
ENTRY(_start)
PHDRS
{
  /* The boot segment is for things like boot code,
   * exception handlers and debug data/routines that all
   * must be in kseg0. It has all permission bits set
   * because it contains code as well as writable data. */
  boot   PT_LOAD FLAGS(7);      /* read-write, executable */
  text   PT_LOAD FLAGS(5);      /* read-only, executable */
  rodata PT_LOAD FLAGS(4);      /* read-only */
  data   PT_LOAD FLAGS(6);      /* read-write */
  dtb    PT_LOAD FLAGS(4);      /* read-only dtb segment */
}
PROVIDE(__tlb_stride_length = 1);
SECTIONS
{
  /* RAM is mapped in kseg0 (cacheable) and kseg1 (non-cacheable) */
  _ram = 0x80000000;

  . = _ram;
  . += 1M;              /* Reserved for Kernel Page Table.  */
  __ebase_phys_start = . - _ram;
  .ebase 0x80100000: AT(__ebase_phys_start) ALIGN(4096)
  {
    __kernel_start = ABSOLUTE(.);
    /* Exception entry points. */
    *(.ebase)
  } : boot

  /*
   * The .boot.* sections contain code and data used by GDB commands.
   * The debugger must always be able to jump to code in this section
   * and to read/write data in this section, so it should be placed
   * in kseg0.
   */
  .boot.text : ALIGN(4096)
  {
    *(.boot.text)
  } : boot

  .boot.data : ALIGN(4096)
  {
    *(.boot.data)
  } : boot 

  /*
   * The .wired.* sections contain code and data that must always be safe
   * to access (i.e. accessing them can't generate exceptions).
   * For now they're placed in kseg0, but when the kernel is moved to
   * kseg2, they should be moved there with it, which will require wiring
   * the mappings for pages containing these sections.
   */
  .wired.text : ALIGN(4096)
  {
    *(.wired.text)
  } : boot

  .wired.data : ALIGN(4096)
  {
    *(.wired.data)
  } : boot

  .text : ALIGN(4096)
  {
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

  .dtb : ALIGN(4)
  {
    __dtb_start = ABSOLUTE(.);
    *(.dtb)
    . = ALIGN(4);
    __dtb_end = ABSOLUTE(.);
  } : rodata

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
  }

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
