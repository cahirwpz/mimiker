#include <common.h>
#include <stdc.h>
#include <elf/mips_elf.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <thread.h>

extern uint8_t _binary_prog_uelf_start[];
extern uint8_t _binary_prog_uelf_size[];
extern uint8_t _binary_prog_uelf_end[];

int exec(){
    uint8_t* elf_image = _binary_prog_uelf_start;
    size_t elf_size = (size_t)(void*)_binary_prog_uelf_size;

    kprintf("[exec] User ELF size: %ld\n", elf_size);

    if(elf_size < sizeof(Elf32_Ehdr)/sizeof(uint8_t)){
        kprintf("[exec] Exec failed: ELF file is too small to contain a valid header\n");
        return -1;
    }

    const uint8_t header_size = 64;
    const uint8_t stride = 8;
    kprintf("[exec] User ELF header dump:\n");
    for(int8_t i = 0; i < header_size; i++){
        if(i % stride == 0) kprintf("[exec]  ");
        uint8_t byte = elf_image[i];
        kprintf(" %02x", byte);
        if(i % stride == 7) kprintf("\n");
    }

    const Elf32_Ehdr* eh = (Elf32_Ehdr*)elf_image;

    // Start by determining the validity of the elf file.

    // First, check for the magic header.
    if(eh->e_ident[EI_MAG0] != ELFMAG0 ||
       eh->e_ident[EI_MAG1] != ELFMAG1 ||
       eh->e_ident[EI_MAG2] != ELFMAG2 ||
       eh->e_ident[EI_MAG3] != ELFMAG3){
        kprintf("[exec] Exec failed: Incorrect ELF magic number\n");
        return -1;
    }
    // Check ELF class
    if(eh->e_ident[EI_CLASS] != ELFCLASS32){
        kprintf("[exec] Exec failed: Unsupported ELF class (!= ELF32)\n");
        return -1;
    }
    // Check data format endianess
    if(eh->e_ident[EI_DATA] != ELFDATA2LSB){
        kprintf("[exec] Exec failed: ELF file is not low-endian\n");
        return -1;
    }
    // Ignore version and os abi field
    // Check file type
    if(eh->e_type != ET_EXEC){
        kprintf("[exec] Exec failed: ELF is not an executable\n");
        return -1;
    }
    // Check machine architecture field
    if(eh->e_machine != EM_MIPS){
        kprintf("[exec] Exec failed: ELF target architecture is not MIPS\n");
        return -1;
    }

    // Take note of the entry point
    kprintf("[exec] Entry point will be at 0x%08x.\n",
            (unsigned int)eh->e_entry);

    // Ensure prog header size compliance
    if(eh->e_phentsize != sizeof(Elf32_Phdr)/sizeof(uint8_t)){
        kprintf("[exec] Exec failed: ELF uses non-standard program header size\n");
        return -1;
    }

    // TODO: Take care of assigning thread/process ASID
    vm_map_t* vmap = vm_map_new(PMAP_USER, 123);
    // Note: we do not claim ownership of the map
    set_active_vm_map(vmap);

    // Iterate over prog headers
    kprintf("[exec] ELF has %d program headers\n", eh->e_phnum);

    const Elf32_Phdr* phs = (Elf32_Phdr*)(elf_image + eh->e_phoff);
    for(uint8_t i = 0; i < eh->e_phnum; i++){
        const Elf32_Phdr* ph = phs + i;
        switch(ph->p_type){
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        default:
            // Ignore the section.
            break;
        case PT_DYNAMIC:
        case PT_INTERP:
            kprintf("[exec] Exec failed: ELF file requests dynamic linking"
                    "by providing a PT_DYNAMIC and/or PT_INTERP segment.\n");
            return -1;
        case PT_SHLIB:
            kprintf("[exec] Exec failed: ELF file contains a PT_SHLIB segment\n");
            return -1;
        case PT_LOAD:
            kprintf("[exec] Processing a PT_LOAD segment: VirtAddr = %p, "
                    "Offset = 0x%08x, FileSiz = 0x%08x, MemSiz = 0x%08x, "
                    "Flags = %d\n", (void*)ph->p_vaddr, (unsigned int)ph->p_offset,
                    (unsigned int)ph->p_filesz, (unsigned int)ph->p_memsz,
                    (unsigned int)ph->p_flags);
            if(ph->p_vaddr % PAGESIZE){
                kprintf("[exec] Exec failed: Segment p_vaddr is not page alligned\n");
                return -1;
            }
            vm_addr_t start = ph->p_vaddr;
            vm_addr_t end   = roundup(ph->p_vaddr + ph->p_memsz, PAGESIZE);
            // TODO: What if segments overlap?
            // Temporarily permissive protection.
            vm_map_entry_t* segment = vm_map_add_entry(vmap, start, end,
                                    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXEC);
            // Allocate pages backing this segment.
            segment->object = default_pager->pgr_alloc();
            // Copy data into the segment
            memcpy((uint8_t*)start, elf_image + ph->p_offset, ph->p_filesz);
            // Zero the rest
            if(ph->p_filesz > ph->p_memsz){
                bzero((uint8_t*)start + ph->p_filesz, ph->p_memsz - ph->p_filesz);
            }
            // Apply correct permissions
            vm_prot_t prot = VM_PROT_NONE;
            if(ph->p_flags | PF_R) prot |= VM_PROT_READ;
            if(ph->p_flags | PF_W) prot |= VM_PROT_WRITE;
            if(ph->p_flags | PF_X) prot |= VM_PROT_EXEC;
            // Note: vm_map_protect is not yet implemented, so
            // this will have no effect as of now
            vm_map_protect(vmap, start, end, prot);
        }
    }

    vm_map_dump(vmap);

    // We need to correct the thread structure so that it can hold the
    // correct $gp value for this execution context.
    thread_t* th = thread_self();
    th->td_kctx.gp = 0;
    th->td_kctx.ra = 0xdeadbeef;
    th->td_kctx.pc = eh->e_entry;
    // TODO: Since there is no process structure yet, this call starts
    // the user code in kernel mode, on kernel stack. This will have
    // to be fixed eventually.

    // Forcefully apply changed context.
    // This might be also done by yielding to the scheduler, but it
    // does not work if we are the only thread, and makes debugging
    // harder, as it difficult to pinpoint the exact time when we
    // enter the user code
    kprintf("[exec] Entering e_entry NOW\n");
    thread_t junk;
    ctx_switch(&junk,th);

    // UNREACHABLE

    return -1;
}
