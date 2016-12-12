#include <common.h>
#include <exec.h>
#include <stdc.h>
#include <elf/mips_elf.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <thread.h>
#include <string.h>
#include <errno.h>
#include <sync.h>
#include <filedesc.h>
#include <vfs_syscalls.h>
#include <mips/stack.h>

#define EMBED_ELF_DECLARE(name)                                                \
  extern uint8_t _binary_##name##_uelf_start[];                                \
  extern uint8_t _binary_##name##_uelf_size[];                                 \
  extern uint8_t _binary_##name##_uelf_end[];

EMBED_ELF_DECLARE(prog);
EMBED_ELF_DECLARE(misbehave);
EMBED_ELF_DECLARE(fd_test);

int get_elf_image(const exec_args_t *args, uint8_t **out_image,
                  size_t *out_size) {

#define EMBED_ELF_BY_NAME(name)                                                \
  if (strcmp(args->prog_name, #name) == 0) {                                   \
    *out_image = _binary_##name##_uelf_start;                                  \
    *out_size = (size_t)_binary_##name##_uelf_size;                            \
    return 0;                                                                  \
  }

  EMBED_ELF_BY_NAME(prog);
  EMBED_ELF_BY_NAME(misbehave);
  EMBED_ELF_BY_NAME(fd_test);
  return -ENOENT;
}

int do_exec(const exec_args_t *args) {
  log("Loading user ELF: %s", args->prog_name);

  uint8_t *elf_image;
  size_t elf_size;
  int n = get_elf_image(args, &elf_image, &elf_size);

  if (n < 0) {
    log("Exec failed: Failed to access program image '%s'", args->prog_name);
    return -ENOENT;
  }

  log("User ELF size: %zu", elf_size);

  if (elf_size < sizeof(Elf32_Ehdr)) {
    log("Exec failed: ELF file is too small to contain a valid header");
    return -ENOEXEC;
  }

  const Elf32_Ehdr *eh = (Elf32_Ehdr *)elf_image;

  /* Start by determining the validity of the elf file. */

  /* First, check for the magic header. */
  if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1 ||
      eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3) {
    log("Exec failed: Incorrect ELF magic number");
    return -ENOEXEC;
  }
  /* Check ELF class */
  if (eh->e_ident[EI_CLASS] != ELFCLASS32) {
    log("Exec failed: Unsupported ELF class (!= ELF32)");
    return -EINVAL;
  }
  /* Check data format endianess */
  if (eh->e_ident[EI_DATA] != ELFDATA2LSB) {
    log("Exec failed: ELF file is not low-endian");
    return -EINVAL;
  }
  /* Ignore version and os abi field */
  /* Check file type */
  if (eh->e_type != ET_EXEC) {
    log("Exec failed: ELF is not an executable");
    return -EINVAL;
  }
  /* Check machine architecture field */
  if (eh->e_machine != EM_MIPS) {
    log("[exec] Exec failed: ELF target architecture is not MIPS");
    return -EINVAL;
  }

  /* Take note of the entry point */
  log("Entry point will be at 0x%08x.", (unsigned int)eh->e_entry);

  /* Ensure minimal prog header size */
  if (eh->e_phentsize < sizeof(Elf32_Phdr)) {
    log("Exec failed: ELF uses too small program headers");
    return -ENOEXEC;
  }

  /*
   * We can not destroy the current vm map, because exec can still fail,
   * and in that case we must be able to return to the original address space.
   */
  vm_map_t *vmap = vm_map_new();
  vm_map_t *old_vmap = vm_map_activate(vmap);

  /* Iterate over prog headers */
  log("ELF has %d program headers", eh->e_phnum);

  const uint8_t *phs_base = elf_image + eh->e_phoff;

  for (uint8_t i = 0; i < eh->e_phnum; i++) {
    const Elf32_Phdr *ph = (Elf32_Phdr *)(phs_base + i * eh->e_phentsize);
    switch (ph->p_type) {
    case PT_NULL:
    case PT_NOTE:
    case PT_PHDR:
    default:
      /* Ignore the section. */
      break;
    case PT_DYNAMIC:
    case PT_INTERP:
      log("Exec failed: ELF file requests dynamic linking"
          "by providing a PT_DYNAMIC and/or PT_INTERP segment.");
      goto exec_fail;
    case PT_SHLIB:
      log("Exec failed: ELF file contains a PT_SHLIB segment");
      goto exec_fail;
    case PT_LOAD:
      log("Processing a PT_LOAD segment: VirtAddr = %p, "
          "Offset = 0x%08x, FileSiz = 0x%08x, MemSiz = 0x%08x, Flags = %d",
          (void *)ph->p_vaddr, (unsigned int)ph->p_offset,
          (unsigned int)ph->p_filesz, (unsigned int)ph->p_memsz,
          (unsigned int)ph->p_flags);
      if (ph->p_vaddr % PAGESIZE) {
        log("Exec failed: Segment p_vaddr is not page alligned");
        goto exec_fail;
      }
      if (ph->p_memsz == 0) {
        /* Avoid creating empty vm_map entries for segments that
           occupy no space in memory, as they might overlap with
           subsequent segments. */
        continue;
      }
      vm_addr_t start = ph->p_vaddr;
      vm_addr_t end = roundup(ph->p_vaddr + ph->p_memsz, PAGESIZE);
      /* TODO: What if segments overlap? */
      /* Temporarily permissive protection. */
      vm_map_entry_t *segment = vm_map_add_entry(
        vmap, start, end, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC);
      /* Allocate pages backing this segment. */
      segment->object = default_pager->pgr_alloc();
      /* Copy data into the segment */
      memcpy((uint8_t *)start, elf_image + ph->p_offset, ph->p_filesz);
      /* Zero the rest */
      if (ph->p_filesz < ph->p_memsz) {
        bzero((uint8_t *)start + ph->p_filesz, ph->p_memsz - ph->p_filesz);
      }
      /* Apply correct permissions */
      vm_prot_t prot = VM_PROT_NONE;
      if (ph->p_flags | PF_R)
        prot |= VM_PROT_READ;
      if (ph->p_flags | PF_W)
        prot |= VM_PROT_WRITE;
      if (ph->p_flags | PF_X)
        prot |= VM_PROT_EXEC;
      /* Note: vm_map_protect is not yet implemented, so
       * this will have no effect as of now */
      vm_map_protect(vmap, start, end, prot);
    }
  }

  /* Create a stack segment. As for now, the stack size is fixed and
   * will not grow on-demand. Also, the stack info should be saved
   * into the thread structure.
   * Generally, the stack should begin at a high address (0x80000000),
   * excluding env vars and arguments, but I've temporarly moved it
   * a bit lower so that it is easier to spot invalid memory access
   * when the stack underflows.
   */
  vm_addr_t stack_bottom = 0x70000000;
  const size_t stack_size = PAGESIZE * 2;

  vm_addr_t stack_start = stack_bottom - stack_size;
  vm_addr_t stack_end = stack_bottom;
  /* TODO: What if this area overlaps with a loaded segment? */
  vm_map_entry_t *stack_segment = vm_map_add_entry(
    vmap, stack_start, stack_end, VM_PROT_READ | VM_PROT_WRITE);
  stack_segment->object = default_pager->pgr_alloc();

  /* Prepare program stack, which includes storing program args... */
  log("Stack real bottom at %p", (void *)stack_bottom);
  prepare_program_stack(args, &stack_bottom);

  thread_t *td = thread_self();
  /* ... file descriptor table ... */
  /* TODO: Copy/share file descriptor table! */
  td->td_fdt = file_desc_table_init();
  /* Before we have a working fork, let's initialize file descriptors required
     by the standard library. */
  int ignore;
  do_open(td, "/dev/uart", 0, O_RDONLY, &ignore);
  do_open(td, "/dev/uart", 0, O_WRONLY, &ignore);
  do_open(td, "/dev/uart", 0, O_WRONLY, &ignore);

  /* ... and user context. */
  uctx_init(thread_self(), eh->e_entry, stack_bottom);

  /*
   * At this point we are certain that exec suceeds.
   * We can safely destroy the previous vm map.
   *
   * One can use do_exec() to start new user program from kernel space,
   * in such case there is no old user vm space to dismantle.
   */
  if (old_vmap)
    vm_map_delete(old_vmap);

  vm_map_dump(vmap);

  log("Entering e_entry NOW");
  user_exc_leave();

  /*NOTREACHED*/
  __builtin_unreachable();

exec_fail:
  /* Return to the previous map, unmodified by exec. */
  vm_map_activate(old_vmap);
  /* Destroy the vm map we began preparing. */
  vm_map_delete(vmap);

  return -EINVAL;
}
