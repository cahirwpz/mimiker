#include <common.h>
#include <exec.h>
#include <stdc.h>
#include <elf/mips_elf.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <thread.h>
#include <string.h>
#include <errno.h>

extern uint8_t _binary_prog_uelf_start[];
extern uint8_t _binary_prog_uelf_size[];
extern uint8_t _binary_prog_uelf_end[];

int get_elf_image(const exec_args_t *args, uint8_t **out_image,
                  size_t *out_size) {
  if (strcmp(args->prog_name, "prog") == 0) {
    *out_image = _binary_prog_uelf_start;
    *out_size = (size_t)_binary_prog_uelf_size;
    return 0;
  } else {
    return -ENOENT;
  }
}

/* Places program args onto the stack.
 * Also modifies value pointed by stack_bottom_p to reflect on changed
 * stack bottom address.
 * The stack layout will be as follows:
 *
 *  ----------- stack segment high address
 *  | argv[n] |
 *  |   ...   |  each of argv[i] is a null-terminated string
 *  | argv[1] |
 *  | argv[0] |
 *  |---------|
 *  | argv    |  the argument vector storing pointers to argv[0..n]
 *  |---------|
 *  | argc    |  a single uint32 declaring the number of arguments (n)
 *  |---------|
 *  | program |
 *  |  stack  |
 *  |   ||    |
 *  |   \/    |
 *  |         |
 *  |   ...   |
 *  ----------- stack segment low address
 *
 *
 * After this function runs, the value pointed by stack_bottom_p will
 * be the address where argc is stored, which is also the bottom of
 * the now empty program stack, so that it can naturally grow
 * downwards.
 */
static void prepare_program_stack(const exec_args_t *args,
                                  vm_addr_t *stack_bottom_p) {
  vm_addr_t stack_end = *stack_bottom_p;
  /* Begin by calculting arguments total size. This has to be done */
  /* in advance, because stack grows downwards. */
  size_t total_arg_size = 0;
  for (size_t i = 0; i < args->argc; i++) {
    total_arg_size += strlen(args->argv[i]) + 1;
  }
  /* Store arguments, creating the argument vector. */
  vm_addr_t arg_vector[args->argc];
  vm_addr_t p = *stack_bottom_p - total_arg_size;
  for (int i = 0; i < args->argc; i++) {
    size_t n = strlen(args->argv[i]) + 1;
    arg_vector[i] = p;
    memcpy((uint8_t *)p, args->argv[i], n);
    p += n;
  }
  assert(p == stack_end);
  /* Move the stack down and word-align it downwards */
  *stack_bottom_p = (*stack_bottom_p - total_arg_size) & 0xfffffffc;

  /* Now, place the argument vector on the stack. */
  size_t arg_vector_size = sizeof(arg_vector);
  vm_addr_t argv = *stack_bottom_p - arg_vector_size;
  memcpy((void *)argv, &arg_vector, arg_vector_size);
  /* Move the stack down. No need to align it again. */
  *stack_bottom_p = *stack_bottom_p - arg_vector_size;

  /* Finally, place argc on the stack */
  *stack_bottom_p = *stack_bottom_p - sizeof(vm_addr_t);
  vm_addr_t *stack_args = (vm_addr_t *)*stack_bottom_p;
  *stack_args = args->argc;

  /* TODO: Environment */
}

int do_exec(const exec_args_t *args) {

  kprintf("[exec] Loading user ELF: %s\n", args->prog_name);

  uint8_t *elf_image;
  size_t elf_size;
  int n = get_elf_image(args, &elf_image, &elf_size);
  if (n < 0) {
    kprintf("[exec] Exec failed: Failed to access program image '%s'\n",
            args->prog_name);
    return -ENOENT;
  }

  kprintf("[exec] User ELF size: %ld\n", elf_size);

  if (elf_size < sizeof(Elf32_Ehdr)) {
    kprintf(
      "[exec] Exec failed: ELF file is too small to contain a valid header\n");
    return -ENOEXEC;
  }

  const Elf32_Ehdr *eh = (Elf32_Ehdr *)elf_image;

  /* Start by determining the validity of the elf file. */

  /* First, check for the magic header. */
  if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1 ||
      eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3) {
    kprintf("[exec] Exec failed: Incorrect ELF magic number\n");
    return -ENOEXEC;
  }
  /* Check ELF class */
  if (eh->e_ident[EI_CLASS] != ELFCLASS32) {
    kprintf("[exec] Exec failed: Unsupported ELF class (!= ELF32)\n");
    return -EINVAL;
  }
  /* Check data format endianess */
  if (eh->e_ident[EI_DATA] != ELFDATA2LSB) {
    kprintf("[exec] Exec failed: ELF file is not low-endian\n");
    return -EINVAL;
  }
  /* Ignore version and os abi field */
  /* Check file type */
  if (eh->e_type != ET_EXEC) {
    kprintf("[exec] Exec failed: ELF is not an executable\n");
    return -EINVAL;
  }
  /* Check machine architecture field */
  if (eh->e_machine != EM_MIPS) {
    kprintf("[exec] Exec failed: ELF target architecture is not MIPS\n");
    return -EINVAL;
  }

  /* Take note of the entry point */
  kprintf("[exec] Entry point will be at 0x%08x.\n", (unsigned int)eh->e_entry);

  /* Ensure minimal prog header size */
  if (eh->e_phentsize < sizeof(Elf32_Phdr)) {
    kprintf("[exec] Exec failed: ELF uses too small program headers\n");
    return -ENOEXEC;
  }

  /* TODO: Get current process description structure */

  /* The current vmap should be taken from the process description! */
  vm_map_t *old_vmap = get_active_vm_map(PMAP_USER);
  /* We may not destroy the current vm map, because exec can still
   * fail, and in that case we must be able to return to the
   * original address space
   */

  /* TODO: Take care of assigning thread/process ASID */
  vm_map_t *vmap = vm_map_new(PMAP_USER, 123);
  /* Note: we do not claim ownership of the map */
  set_active_vm_map(vmap);

  /* Iterate over prog headers */
  kprintf("[exec] ELF has %d program headers\n", eh->e_phnum);

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
      kprintf("[exec] Exec failed: ELF file requests dynamic linking"
              "by providing a PT_DYNAMIC and/or PT_INTERP segment.\n");
      goto exec_fail;
    case PT_SHLIB:
      kprintf("[exec] Exec failed: ELF file contains a PT_SHLIB segment\n");
      goto exec_fail;
    case PT_LOAD:
      kprintf("[exec] Processing a PT_LOAD segment: VirtAddr = %p, "
              "Offset = 0x%08x, FileSiz = 0x%08x, MemSiz = 0x%08x, "
              "Flags = %d\n",
              (void *)ph->p_vaddr, (unsigned int)ph->p_offset,
              (unsigned int)ph->p_filesz, (unsigned int)ph->p_memsz,
              (unsigned int)ph->p_flags);
      if (ph->p_vaddr % PAGESIZE) {
        kprintf("[exec] Exec failed: Segment p_vaddr is not page alligned\n");
        goto exec_fail;
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

  /* Prepare program stack, which includes storing program args. */
  prepare_program_stack(args, &stack_bottom);
  kprintf("[exec] Stack real bottom at %p\n", (void *)stack_bottom);

  vm_map_dump(vmap);

  /* At this point we are certain that exec suceeds.
   * We can safely destroy the previous vm map. */

  /* This condition will be unnecessary when we take the vmap info
   * from thread struct. The user thread calling exec() WILL
   * certainly have an existing vmap before exec()ing. */
  if (old_vmap)
    vm_map_delete(old_vmap);

  /* TODO: Assign the new vm map to the process structure */

  thread_t *th = thread_self();
  th->td_kctx.gp = 0;
  th->td_kctx.pc = eh->e_entry;
  th->td_kctx.sp = stack_bottom;
  /* TODO: Since there is no process structure yet, this call starts
   * the user code in kernel mode. $sp is set according to data
   * prepared by prepare_program_stack. Setting $ra is irrelevant,
   * as the ctx_switch procedure overwrites it anyway. */

  /* Forcefully apply changed context.
   * This might be also done by yielding to the scheduler, but it
   * does not work if we are the only thread, and makes debugging
   * harder, as it difficult to pinpoint the exact time when we
   * enter the user code */
  kprintf("[exec] Entering e_entry NOW\n");
  thread_t junk;
  ctx_switch(&junk, th);

  /*NOTREACHED*/
  __builtin_unreachable();

exec_fail:
  /* Destroy the vm map we began preparing */
  vm_map_delete(vmap);
  /* Return to the previous map, unmodified by exec */
  if (old_vmap)
    set_active_vm_map(old_vmap);

  return -EINVAL;
}
