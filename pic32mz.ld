/*
 * Linker script for PIC32MZ firmware.
 */
OUTPUT_ARCH(mips)
ENTRY(_start)
MEMORY
{
  flash   (rx)   : ORIGIN = 0xbfc00000,	LENGTH = 0xff00
  devcfg  (r)    : ORIGIN = 0xbfc0ffc0,	LENGTH = 16
  devsign (r)    : ORIGIN = 0xbfc0ffe0,	LENGTH = 16
  prog    (rx)   : ORIGIN = 0xbd000000,	LENGTH = 2048K
  ram     (rw!x) : ORIGIN = 0xa0000000,	LENGTH = 512K
}

SECTIONS
{
  /* higher address of the user mode stack */
  _estack = ORIGIN(ram) + LENGTH(ram);

  .text : AT(ORIGIN(flash))
  {
    _text = ABSOLUTE(.);
    /* Exception handlers. */
    *(.exception)
    /* Execution starts here. */
    *(.init)
    *(.text .text.*)
    . = ALIGN (4);
    __rodata_start = ABSOLUTE(.);
    *(.rodata .rodata.*)
    __rodata_end = ABSOLUTE(.);
    _etext = ABSOLUTE(.);
  } > flash

  /* Device configuration.  */
  .devcfg : AT(ORIGIN(devcfg))
  {
    *(.config3)
    *(.config2)
    *(.config1)
    *(.config0)
  } > devcfg

  .devsign : AT(ORIGIN(devsign))
  {
    *(.devsign)
  } > devsign

  /* Start data (internal SRAM).  */
  .data : AT(ADDR(.text) + SIZEOF(.text))
  {
    __data_start = ABSOLUTE(.);
    _data = ABSOLUTE(.);
    _gp = .; 	/* We have only 32k RAM on MC-24, so no need for 0x8000 offset. */
    *(.data .data.*)
    /* We want the small data sections together, so single-instruction offsets
       can access them all, and initialized data all before uninitialized, so
       we can shorten the on-disk segment size.  */
    *(.sdata .sdata.*)
    . = ALIGN(4);
    _edata = ABSOLUTE(.);
  }

  .bss ADDR (.data) + SIZEOF (.data) (NOLOAD) :
  {
    _bss = ABSOLUTE(.);
    __bss_start = ABSOLUTE(.);
    *(.sbss .scommon)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN (4);
    __bss_end = ABSOLUTE(.);
    _ebss = ABSOLUTE(.);
  }

  _end = .;

  /DISCARD/ :
  {
    *(.gnu.attributes)
    *(.comment)
    *(.MIPS.abiflags)
  }
}
