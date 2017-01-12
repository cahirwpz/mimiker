/* Linker script for OVPsim SmartLoaderLinux firmware. */
OUTPUT_ARCH(mips)
ENTRY(_start)
PHDRS
{
  text PT_LOAD FLAGS(5); /* read-only, executable */
  data PT_LOAD FLAGS(6); /* read-write */
  initrd PT_NULL FLAGS(6); /* read-write */
}
PROVIDE(__tlb_stride_length = 1);
PROVIDE(__initrd_pages = 5);
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
    /* Text and read-only data follows. */
    __text = ABSOLUTE(.);
    *(.text .text.*)
    . = ALIGN (4);
    *(.rodata .rodata.*)
    __etext = ABSOLUTE(.);
  } : text

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

  _ramdisk_size = 4096*__initrd_pages;

  .initrd :
  {
    . = ALIGN(4096);
    __initrd = ABSOLUTE(.);
    . = . + _ramdisk_size ;
  } : initrd

  .bss :
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
