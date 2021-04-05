#include <amd64/asm.h>
#define ASM_FILE
#include <amd64/multiboot2.h>

#define MULTIBOOT_HEADER_LENGTH (multiboot_header_end - multiboot_header_start)
#define MULTIBOOT_HEADER_CHECKSUM \
  (-(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + \
     MULTIBOOT_HEADER_LENGTH))

	.align MULTIBOOT_HEADER_ALIGN

multiboot_header_start:

	.long MULTIBOOT2_HEADER_MAGIC
	.long MULTIBOOT_ARCHITECTURE_I386
	.long MULTIBOOT_HEADER_LENGTH
	.long MULTIBOOT_HEADER_CHECKSUM

	# start of the tag list

	# TODO(MichalBlk): request some information (e.g. meminfo, VBE...)

	# end of the tag list

	.align 8
	.short MULTIBOOT_HEADER_TAG_END
	.short 0
	.long 8     # sizeof(struct multiboot_tag)	

multiboot_header_end:

	.code32

_ENTRY(_start)
	jmp _start
_END(_start)

# vim: sw=8 ts=8 et
