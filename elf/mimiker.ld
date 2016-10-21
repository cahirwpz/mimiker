OUTPUT_ARCH(mips)
ENTRY(main)

PHDRS
{
    text   PT_LOAD FLAGS(5); /* read-only, executable */
    data   PT_LOAD FLAGS(6); /* read-write */
    rodata PT_LOAD FLAGS(4); /* read-only */
}
SECTIONS
{
    .text 0x00001000:
    {
        *(.text .text.*)
        . = ALIGN(4096);
    } : text
    
    .data ADDR(.text) + SIZEOF (.text):
    {     
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(4);
    } : data

    .rodata ADDR(.data) + SIZEOF(.data):
    {
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } : rodata

    .bss ADDR (.rodata) + SIZEOF (.rodata) (NOLOAD) :
    {
        *(.bss .bss.*)
    }
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

