#define KL_LOG KL_USER
#include <klog.h>
#include <common.h>
#include <exec.h>
#include <stdc.h>
#include <elf/mips_elf.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <thread.h>
#include <errno.h>
#include <filedesc.h>
#include <sbrk.h>
#include <vfs.h>
#include <stack.h>
#include <mount.h>
#include <vnode.h>
#include <proc.h>

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
    vm_map_entry_t *sbrk;
    vm_addr_t sbrk_end;
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
          klog("Exec failed: Segment p_vaddr is not page alligned");
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

        /* Read data from file into the segment */
        /* TODO: This is a lot of copying! Ideally we would look up the
           vm_object associated with the elf vnode, create a shadow vm_object on
           top of it using correct size/offset, and we would use it to page the
           file contents on demand. But we don't have a vnode_pager yet. */
        uio = UIO_SINGLE_KERNEL(UIO_READ, ph->p_offset, (char *)start,
                                ph->p_filesz);
        error = VOP_READ(elf_vnode, &uio);
        if (error < 0) {
          klog("Exec failed: Elf file reading failed.");
          goto exec_fail;
        }
        assert(uio.uio_resid == 0);

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
  const size_t stack_size = 8 * 1024 * 1024; /* 8 MiB */

  vm_addr_t stack_start = stack_bottom - stack_size;
  vm_addr_t stack_end = stack_bottom;
  /* TODO: What if this area overlaps with a loaded segment? */
  vm_map_entry_t *stack_segment = vm_map_add_entry(
    vmap, stack_start, stack_end, VM_PROT_READ | VM_PROT_WRITE);
  stack_segment->object = default_pager->pgr_alloc();

  /* Prepare program stack, which includes storing program args. */
  klog("Stack real bottom at %p", (void *)stack_bottom);
  stack_user_entry_setup(args, &stack_bottom);

  /* Set up user context. */
  exc_frame_init(td->td_uframe, (void *)eh.e_entry, (void *)stack_bottom,
                 EF_USER);

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

  assert(td->td_proc == NULL);

  klog("Starting program \"%s\"", prog->argv[0]);

  /* This thread will become a main thread of newly created user process. */
  proc_t *p = proc_create();
  proc_populate(p, td);

  /* Let's assign an empty virtual address space, to be filled by `do_exec` */
  p->p_uspace = vm_map_new();

  /* Prepare file descriptor table... */
  fdtab_t *fdt = fdtab_alloc();
  fdtab_ref(fdt);
  td->td_proc->p_fdtable = fdt;

  /* ... and initialize file descriptors required by the standard library. */
  int ignore;
  do_open(td, "/dev/cons", O_RDONLY, 0, &ignore);
  do_open(td, "/dev/cons", O_WRONLY, 0, &ignore);
  do_open(td, "/dev/cons", O_WRONLY, 0, &ignore);

  if (do_exec(prog) != -EJUSTRETURN)
    panic("Failed to start %s program.", prog->argv[0]);

  user_exc_leave();
}
