#define KL_LOG KL_USER
#include <klog.h>
#include <common.h>
#include <exec.h>
#include <stdc.h>
#include <elf/mips_elf.h>
#include <vm_map.h>
#include <vm_object.h>
#include <thread.h>
#include <errno.h>
#include <filedesc.h>
#include <sbrk.h>
#include <vfs.h>
#include <machine/ustack.h>
#include <mount.h>
#include <vnode.h>
#include <proc.h>
#include <systm.h>

/*! \brief Stores C-strings in ustack and makes stack-allocated pointers
 *  point on them.
 *
 * \return ENOMEM if there was not enough space on ustack */
static int store_strings(ustack_t *us, const char **strv, char **stack_strv,
                         size_t howmany) {
  int error;
  /* Store arguments, creating the argument vector. */
  for (size_t i = 0; i < howmany; i++) {
    size_t n = strlen(strv[i]);
    if ((error = ustack_alloc_string(us, n, &stack_strv[i])))
      return error;
    memcpy(stack_strv[i], strv[i], n + 1);
  }
  return 0;
}

/*!\brief Places program args onto the stack.
 *
 * Also modifies value pointed by stack_bottom_p to reflect on changed stack
 * bottom address.  The stack layout will be as follows:
 *
 *  ----------- stack segment high address
 *  | envp[m-1]|
 *  |   ...    |  each of envp[i] is a null-terminated string
 *  | envp[1]  |
 *  | envp[0]  |
 *  |----------|
 *  | argv[n-1]|
 *  |   ...    |  each of argv[i] is a null-terminated string
 *  | argv[1]  |
 *  | argv[0]  |
 *  |----------|
 *  |          |
 *  |  envp    |  NULL-terminated environment vector
 *  |          |  storing pointers to envp[0..m]
 *  |----------|
 *  |          |
 *  |  argv    |  NULL-terminated argument vector
 *  |          |  storing pointers to argv[0..n]
 *  |----------|
 *  |  argc    |  a single uint32 declaring the number of arguments (n)
 *  |----------|
 *  |  program |
 *  |   stack  |
 *  |    ||    |
 *  |    \/    |
 *  |          |
 *  |    ...   |
 *  ----------- stack segment low address
 * Here argc is n and both argv[n] and argc[m] store a NULL-pointer.
 * (see System V ABI MIPS RISC Processor Supplement, 3rd edition, p. 30)
 *
 * After this function runs, the value pointed by stack_bottom_p will be the
 * address where argc is stored, which is also the bottom of the now empty
 * program stack, so that it can naturally grow downwards.
 */
static int user_entry_setup(const exec_args_t *args, vaddr_t *stack_top_p) {
  ustack_t us;
  char **argv, **envp;
  int error;
  size_t argc = 0, envc = 0;

  while (args->argv[argc] != NULL)
    argc++;
  while (args->envp[envc] != NULL)
    envc++;

  assert(argc > 0);

  ustack_setup(&us, *stack_top_p, ARG_MAX);

  if ((error = ustack_push_int(&us, argc)))
    goto fail;
  if ((error = ustack_alloc_ptr_n(&us, argc, (vaddr_t *)&argv)))
    goto fail;
  if ((error = ustack_push_long(&us, (long)NULL)))
    goto fail;
  if ((error = ustack_alloc_ptr_n(&us, envc, (vaddr_t *)&envp)))
    goto fail;
  if ((error = ustack_push_long(&us, (long)NULL)))
    goto fail;

  if ((error = store_strings(&us, args->argv, argv, argc)))
    goto fail;
  if ((error = store_strings(&us, args->envp, envp, envc)))
    goto fail;

  ustack_finalize(&us);

  for (size_t i = 0; i < argc; i++)
    ustack_relocate_ptr(&us, (vaddr_t *)&argv[i]);
  for (size_t i = 0; i < envc; i++)
    ustack_relocate_ptr(&us, (vaddr_t *)&envp[i]);

  error = ustack_copy(&us, stack_top_p);

fail:
  ustack_teardown(&us);
  return error;
}

int do_exec(const exec_args_t *args) {
  thread_t *td = thread_self();

  assert(td->td_proc != NULL);

  klog("Loading user ELF: %s", args->prog_name);

  vnode_t *elf_vnode;
  int error = vfs_lookup(args->prog_name, &elf_vnode);
  if (error == -ENOENT) {
    klog("Exec failed: No such file '%s'", args->prog_name);
    return -ENOENT;
  } else if (error < 0) {
    klog("Exec failed: Failed to access file '%s'", args->prog_name);
    return error;
  }

  vattr_t elf_attr;
  error = VOP_GETATTR(elf_vnode, &elf_attr);
  if (error)
    return error;
  size_t elf_size = elf_attr.va_size;

  klog("User ELF size: %u", elf_size);

  if (elf_size < sizeof(Elf32_Ehdr)) {
    klog("Exec failed: ELF file is too small to contain a valid header");
    return -ENOEXEC;
  }

  Elf32_Ehdr eh;
  uio_t uio;

  uio = UIO_SINGLE_KERNEL(UIO_READ, 0, &eh, sizeof(Elf32_Ehdr));
  error = VOP_READ(elf_vnode, &uio);
  if (error < 0) {
    klog("Exec failed: Elf file reading failed.");
    return error;
  }
  assert(uio.uio_resid == 0);

  /* Start by determining the validity of the elf file. */

  /* First, check for the magic header. */
  if (eh.e_ident[EI_MAG0] != ELFMAG0 || eh.e_ident[EI_MAG1] != ELFMAG1 ||
      eh.e_ident[EI_MAG2] != ELFMAG2 || eh.e_ident[EI_MAG3] != ELFMAG3) {
    klog("Exec failed: Incorrect ELF magic number");
    return -ENOEXEC;
  }
  /* Check ELF class */
  if (eh.e_ident[EI_CLASS] != ELFCLASS32) {
    klog("Exec failed: Unsupported ELF class (!= ELF32)");
    return -EINVAL;
  }
  /* Check data format endianess */
  if (eh.e_ident[EI_DATA] != ELFDATA2LSB) {
    klog("Exec failed: ELF file is not low-endian");
    return -EINVAL;
  }
  /* Ignore version and os abi field */
  /* Check file type */
  if (eh.e_type != ET_EXEC) {
    klog("Exec failed: ELF is not an executable");
    return -EINVAL;
  }
  /* Check machine architecture field */
  if (eh.e_machine != EM_MIPS) {
    klog("[exec] Exec failed: ELF target architecture is not MIPS");
    return -EINVAL;
  }

  /* Take note of the entry point */
  klog("Entry point will be at 0x%08x.", (unsigned int)eh.e_entry);

  /* Ensure minimal prog header size */
  if (eh.e_phentsize < sizeof(Elf32_Phdr)) {
    klog("Exec failed: ELF uses too small program headers");
    return -ENOEXEC;
  }

  /* We assume process may only have a single thread. But if there were more
     than one thread in the process that called exec, all other threads must be
     forcefully terminated. */
  proc_t *p = td->td_proc;

  /*
   * We can not destroy the current vm map, because exec can still fail,
   * and in that case we must be able to return to the original address space.
   */
  struct {
    vm_map_t *uspace;
    vm_segment_t *sbrk;
    vaddr_t sbrk_end;
  } old = {p->p_uspace, p->p_sbrk, p->p_sbrk_end};

  /* We are the only live thread in this process. We can safely give it a new
   * uspace. */
  vm_map_t *vmap = vm_map_new();

  p->p_uspace = vmap;
  p->p_sbrk = NULL;
  p->p_sbrk_end = 0;
  sbrk_attach(p); /* Attach fresh brk segment. */

  vm_map_activate(vmap);

  /* Iterate over prog headers */
  klog("ELF has %d program headers", eh.e_phnum);

  assert(eh.e_phoff < 64);
  assert(eh.e_phentsize < 128);
  /* We allocate this buffer on stack. It's confirmed to be small. A statically
     allocated buffer won't do, because it would need restrictive
     synchronization. Using a malloc pool would be probably an overkill for this
     case. */
  size_t phs_size = eh.e_phoff * eh.e_phentsize;
  char phs[phs_size];

  /* Read program headers. */
  uio = UIO_SINGLE_KERNEL(UIO_READ, eh.e_phoff, &phs, phs_size);
  error = VOP_READ(elf_vnode, &uio);
  if (error < 0) {
    klog("Exec failed: Elf file reading failed.");
    return error;
  }
  assert(uio.uio_resid == 0);

  for (uint8_t i = 0; i < eh.e_phnum; i++) {
    const Elf32_Phdr *ph = (Elf32_Phdr *)(phs + i * eh.e_phentsize);
    switch (ph->p_type) {
      case PT_NULL:
      case PT_NOTE:
      case PT_PHDR:
      default:
        /* Ignore the section. */
        break;
      case PT_DYNAMIC:
      case PT_INTERP:
        klog("Exec failed: ELF file requests dynamic linking"
             "by providing a PT_DYNAMIC and/or PT_INTERP segment.");
        goto exec_fail;
      case PT_SHLIB:
        klog("Exec failed: ELF file contains a PT_SHLIB segment");
        goto exec_fail;
      case PT_LOAD:
        klog("PT_LOAD: VAddr %08x Offset %08x FileSz %08x MemSz %08x Flags %d",
             (void *)ph->p_vaddr, (unsigned)ph->p_offset,
             (unsigned)ph->p_filesz, (unsigned)ph->p_memsz,
             (unsigned)ph->p_flags);
        if (ph->p_vaddr % PAGESIZE) {
          klog("Exec failed: Segment p_vaddr is not page aligned!");
          goto exec_fail;
        }
        if (ph->p_memsz == 0) {
          /* Avoid creating empty vm_map entries for segments that
             occupy no space in memory, as they might overlap with
             subsequent segments. */
          continue;
        }
        vaddr_t start = ph->p_vaddr;
        vaddr_t end = roundup(ph->p_vaddr + ph->p_memsz, PAGESIZE);
        /* Temporarily permissive protection. */
        vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
        vm_segment_t *seg = vm_segment_alloc(
          obj, start, end, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC);
        error = vm_map_insert(vmap, seg, VM_FIXED);
        /* TODO: What if segments overlap? */
        assert(error == 0);

        /* Read data from file into the segment */
        if (ph->p_filesz > 0) {
          /* TODO: This is a lot of copying! Ideally we would look up the
           * vm_object associated with the elf vnode, create a shadow vm_object
           * on top of it using correct size/offset, and we would use it to page
           * the file contents on demand. But we don't have a vnode_pager yet.
           */
          uio = UIO_SINGLE_KERNEL(UIO_READ, ph->p_offset, (char *)start,
                                  ph->p_filesz);
          error = VOP_READ(elf_vnode, &uio);
          if (error < 0) {
            klog("Exec failed: Elf file reading failed.");
            goto exec_fail;
          }
          assert(uio.uio_resid == 0);
        }

        /* Zero the rest */
        if (ph->p_filesz < ph->p_memsz)
          bzero((uint8_t *)start + ph->p_filesz, ph->p_memsz - ph->p_filesz);
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
  vaddr_t stack_top = 0x7f800000;
  vaddr_t stack_limit = 0x7f000000; /* stack size is 8 MiB */

  vm_object_t *stack_obj = vm_object_alloc(VM_ANONYMOUS);
  vm_segment_t *stack_seg = vm_segment_alloc(stack_obj, stack_limit, stack_top,
                                             VM_PROT_READ | VM_PROT_WRITE);
  error = vm_map_insert(vmap, stack_seg, VM_FIXED);
  /* TODO: What if this area overlaps with a loaded segment? */
  assert(error == 0);

  /* Prepare program stack, which includes storing program args. */
  klog("Stack real top at %p", (void *)stack_top);
  user_entry_setup(args, &stack_top);

  /* Set up user context. */
  exc_frame_init(td->td_uframe, (void *)eh.e_entry, (void *)stack_top, EF_USER);

  /* At this point we are certain that exec succeeds.  We can safely destroy the
   * previous vm map, and permanently assign this one to the current process. */
  vm_map_delete(old.uspace);

  vm_map_dump(vmap);

  klog("Enter userspace with: pc=%p, sp=%p", eh.e_entry, stack_bottom);
  return -EJUSTRETURN;

exec_fail:
  /* Return to the previous map, unmodified by exec. */
  p->p_uspace = old.uspace;
  p->p_sbrk = old.sbrk;
  p->p_sbrk_end = old.sbrk_end;
  vm_map_activate(old.uspace);
  /* Destroy the vm map we began preparing. */
  vm_map_delete(vmap);

  return -EINVAL;
}

noreturn void run_program(const exec_args_t *prog) {
  thread_t *td = thread_self();
  proc_t *p = proc_self();

  assert(p != NULL);

  klog("Starting program \"%s\"", prog->prog_name);

  /* Let's assign an empty virtual address space, to be filled by `do_exec` */
  p->p_uspace = vm_map_new();

  /* Prepare file descriptor table... */
  fdtab_t *fdt = fdtab_alloc();
  fdtab_hold(fdt);
  p->p_fdtable = fdt;

  /* ... and initialize file descriptors required by the standard library. */
  int _stdin, _stdout, _stderr;
  do_open(td, "/dev/cons", O_RDONLY, 0, &_stdin);
  do_open(td, "/dev/cons", O_WRONLY, 0, &_stdout);
  do_open(td, "/dev/cons", O_WRONLY, 0, &_stderr);

  assert(_stdin == 0);
  assert(_stdout == 1);
  assert(_stderr == 2);

  if (do_exec(prog) != -EJUSTRETURN)
    panic("Failed to start %s program.", prog->prog_name);

  user_exc_leave();
}
