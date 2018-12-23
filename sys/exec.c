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

#define PTR_SIZE sizeof(void *)

typedef struct {
  void *data;
  size_t nleft;
} buffer_t;

static inline void buffer_advance(buffer_t *buf, size_t len) {
  buf->data += len;
  buf->nleft -= len;
}

static inline void buffer_align(buffer_t *buf, size_t align) {
  intptr_t last_data = (intptr_t)buf->data;
  buf->data = align(buf->data, align);
  size_t padding = (intptr_t)buf->data - last_data;
  assert(buf->nleft >= padding);
  buf->nleft -= padding;
}

static int buffer_copyin_ptr(buffer_t *buf, char **user_ptr_p) {
  int error;
  if ((error = copyin(user_ptr_p, buf->data, PTR_SIZE)))
    return error;
  buffer_advance(buf, PTR_SIZE);
  return 0;
}

static int buffer_copyin_str(buffer_t *buf, char *user_str, char **str_p) {
  size_t len;
  int error;
  if ((error = copyinstr(user_str, buf->data, buf->nleft, &len)))
    return error;
  *str_p = buf->data;
  buffer_advance(buf, len);
  return 0;
}

static int buffer_copyin_strv(buffer_t *buf, char **user_strv, char ***strv_p) {
  int error;

  buffer_align(buf, PTR_SIZE);
  assert(is_aligned(buf->nleft, PTR_SIZE));

  /* Copy in vector of string pointers. */
  char **strv = buf->data;
  for (int i = 0; buf->nleft > 0; i++) {
    if ((error = buffer_copyin_ptr(buf, user_strv + i)))
      return error;
    if (!strv[i])
      break;
  }

  *strv_p = strv;

  /* Now we have all user-space pointers to strings so copy in the strings. */
  for (int i = 0; strv[i]; i++) {
    if ((error = buffer_copyin_str(buf, strv[i], strv + i)))
      return (error == -ENAMETOOLONG) ? -E2BIG : error;
  }

  return 0;
}

int exec_args_copyin(exec_args_t *exec_args, char *user_path, char **user_argv,
                     char **user_envp) {
  int error;
  buffer_t buf;

  buf = (buffer_t){.data = exec_args->data, .nleft = PATH_MAX};
  if ((error = buffer_copyin_str(&buf, user_path, &exec_args->prog_name)))
    return error;

  buf = (buffer_t){.data = exec_args->data + PATH_MAX, .nleft = ARG_MAX};
  if ((error = buffer_copyin_strv(&buf, user_argv, &exec_args->argv)))
    return error;
  if ((error = buffer_copyin_strv(&buf, user_envp, &exec_args->envp)))
    return error;
  return 0;
}

/*! \brief Stores C-strings in ustack and makes stack-allocated pointers
 *  point on them.
 *
 * \return ENOMEM if there was not enough space on ustack */
static int store_strings(ustack_t *us, char **strv, char **stack_strv,
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
 * Also modifies value pointed by stack_top_p to reflect on changed stack
 * bottom address.  The stack layout will be as follows:
 *
 *  +----------+ stack segment high address
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
 *  +----------+ stack segment low address
 *
 * Here argc is n and both argv[n] and argc[m] store a NULL-pointer.
 * (see System V ABI MIPS RISC Processor Supplement, 3rd edition, p. 30)
 *
 * After this function runs, the value pointed by stack_top_p will be the
 * address where argc is stored, which is also the bottom of the now empty
 * program stack, so that it can naturally grow downwards.
 */
static int exec_args_copyout(const exec_args_t *args, vaddr_t *stack_top_p) {
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

  if ((error = ustack_push_int(&us, argc)) ||
      (error = ustack_alloc_ptr_n(&us, argc, (vaddr_t *)&argv)) ||
      (error = ustack_push_long(&us, (long)NULL)) ||
      (error = ustack_alloc_ptr_n(&us, envc, (vaddr_t *)&envp)) ||
      (error = ustack_push_long(&us, (long)NULL)) ||
      (error = store_strings(&us, args->argv, argv, argc)) ||
      (error = store_strings(&us, args->envp, envp, envc)))
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

static int open_executable(const char *path, vnode_t **vn_p, vattr_t *attr) {
  vnode_t *vn = *vn_p;
  int error;

  klog("Loading program: %s", path);

  /* Translate program name to vnode. */
  if ((error = vfs_lookup(path, &vn)))
    return error;

  if ((error = VOP_GETATTR(vn, attr)))
    return error;

  /* It must be a regular executable file with non-zero size. */
  if (vn->v_type != V_REG)
    return -EACCES;
  if (attr->va_size == 0)
    return -ENOEXEC;
  if ((error = VOP_ACCESS(vn, VEXEC)))
    return error;

  /* TODO Some checks are missing:
   * 1) Check if NOEXEC bit is set on the filesystem this file resides on.
   * 2) If file is opened for write then return ETXTBSY */

  *vn_p = vn;
  return 0;
}

static int check_elf(vnode_t *vn, vattr_t *attr, Elf32_Ehdr *eh) {
  int error;

  size_t elf_size = attr->va_size;
  klog("User ELF size: %u", elf_size);

  if (elf_size < sizeof(Elf32_Ehdr)) {
    klog("Exec failed: ELF file is too small to contain a valid header");
    return -ENOEXEC;
  }

  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 0, eh, sizeof(Elf32_Ehdr));
  if ((error = VOP_READ(vn, &uio)) < 0) {
    klog("Exec failed: Reading ELF header failed.");
    return error;
  }
  assert(uio.uio_resid == 0);

  /* Start by determining the validity of the elf file. */

  /* First, check for the magic header. */
  if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1 ||
      eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3) {
    klog("Exec failed: Incorrect ELF magic number");
    return -ENOEXEC;
  }
  /* Check ELF class */
  if (eh->e_ident[EI_CLASS] != ELFCLASS32) {
    klog("Exec failed: Unsupported ELF class (!= ELF32)");
    return -EINVAL;
  }
  /* Check data format endianess */
  if (eh->e_ident[EI_DATA] != ELFDATA2LSB) {
    klog("Exec failed: ELF file is not low-endian");
    return -EINVAL;
  }
  /* Ignore version and os abi field */
  /* Check file type */
  if (eh->e_type != ET_EXEC) {
    klog("Exec failed: ELF is not an executable");
    return -EINVAL;
  }
  /* Check machine architecture field */
  if (eh->e_machine != EM_MIPS) {
    klog("Exec failed: ELF target architecture is not MIPS");
    return -EINVAL;
  }

  /* Take note of the entry point */
  klog("Entry point will be at 0x%08x.", (unsigned int)eh->e_entry);

  /* Ensure minimal prog header size */
  if (eh->e_phentsize < sizeof(Elf32_Phdr)) {
    klog("Exec failed: ELF uses too small program headers");
    return -ENOEXEC;
  }

  return 0;
}

static int load_elf_segment(proc_t *p, vnode_t *vn, Elf32_Phdr *ph) {
  int error;

  /* Avoid creating empty vm_map entries for segments that occupy no space in
   * memory, as they might overlap with subsequent segments. */
  if (ph->p_memsz == 0)
    return 0;

  klog("PT_LOAD: VAddr %08x Offset %08x FileSz %08x MemSz %08x Flags %d",
       (void *)ph->p_vaddr, (unsigned)ph->p_offset, (unsigned)ph->p_filesz,
       (unsigned)ph->p_memsz, (unsigned)ph->p_flags);

  if (!is_aligned(ph->p_vaddr, PAGESIZE)) {
    klog("Exec failed: Segment virtual address is not page aligned!");
    return -ENOEXEC;
  }

  vaddr_t start = ph->p_vaddr;
  vaddr_t end = roundup(ph->p_vaddr + ph->p_memsz, PAGESIZE);

  /* Temporarily permissive protection. */
  vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
  vm_segment_t *seg = vm_segment_alloc(
    obj, start, end, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC);
  error = vm_map_insert(p->p_uspace, seg, VM_FIXED);
  /* TODO: What if segments overlap? */
  assert(error == 0);

  /* Read data from file into the segment */
  if (ph->p_filesz > 0) {
    /* TODO: This is a lot of copying! Ideally we would look up the
     * vm_object associated with the elf vnode, create a shadow vm_object
     * on top of it using correct size/offset, and we would use it to page
     * the file contents on demand. But we don't have a vnode_pager yet.
     */
    uio_t uio =
      UIO_SINGLE_KERNEL(UIO_READ, ph->p_offset, (char *)start, ph->p_filesz);
    if ((error = VOP_READ(vn, &uio)) < 0) {
      klog("Exec failed: Reading ELF segment failed.");
      return error;
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
  vm_map_protect(p->p_uspace, start, end, prot);
  return 0;
}

static int load_elf(proc_t *p, vnode_t *vn, Elf32_Ehdr *eh) {
  int error;

  assert(eh->e_phoff < 64);
  assert(eh->e_phentsize < 128);

  /* Read in program headers. */
  size_t phs_size = eh->e_phoff * eh->e_phentsize;
  char *phs = kmalloc(M_TEMP, phs_size, 0);
  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, eh->e_phoff, phs, phs_size);
  if ((error = VOP_READ(vn, &uio)) < 0) {
    klog("Exec failed: Reading program headers failed.");
    goto fail;
  }
  assert(uio.uio_resid == 0);

  /* Iterate over program headers */
  klog("ELF has %d program headers", eh->e_phnum);
  for (int i = 0; i < eh->e_phnum; i++) {
    Elf32_Phdr *ph = (Elf32_Phdr *)(phs + i * eh->e_phentsize);
    switch (ph->p_type) {
      case PT_LOAD:
        if ((error = load_elf_segment(p, vn, ph)))
          goto fail;
        break;
      case PT_DYNAMIC:
      case PT_INTERP:
        klog("Exec failed: ELF file requests dynamic linking"
             "by providing a PT_DYNAMIC and/or PT_INTERP segment.");
        error = -ENOEXEC;
        goto fail;
      case PT_SHLIB:
        klog("Exec failed: ELF file contains a PT_SHLIB segment");
        error = -ENOEXEC;
        goto fail;
      /* Ignore following sections. */
      case PT_NULL:
      case PT_NOTE:
      case PT_PHDR:
      default:
        break;
    }
  }

fail:
  kfree(M_TEMP, phs);
  return error;
}

typedef struct exec_vmspace {
  vm_map_t *uspace;
  vm_segment_t *sbrk;
  vaddr_t sbrk_end;
} exec_vmspace_t;

/* stack size is 8 MiB */
#define MIPS_STACK_TOP 0x7f800000
#define MIPS_STACK_LIMIT 0x7f000000

static void enter_new_vmspace(proc_t *p, exec_vmspace_t *saved) {
  saved->uspace = p->p_uspace;
  saved->sbrk = p->p_sbrk;
  saved->sbrk_end = p->p_sbrk_end;

  /* We are the only live thread in this process.
   * We can safely give it a new uspace. */
  p->p_uspace = vm_map_new();

  /* Attach fresh brk segment. */
  p->p_sbrk = NULL;
  p->p_sbrk_end = 0;
  sbrk_attach(p);

  /* Create a stack segment. As for now, the stack size is fixed and
   * will not grow on-demand. Also, the stack info should be saved
   * into the thread structure.
   * Generally, the stack should begin at a high address (0x80000000),
   * excluding env vars and arguments, but I've temporarly moved it
   * a bit lower so that it is easier to spot invalid memory access
   * when the stack underflows.
   */
  vm_object_t *stack_obj = vm_object_alloc(VM_ANONYMOUS);
  vm_segment_t *stack_seg = vm_segment_alloc(
    stack_obj, MIPS_STACK_LIMIT, MIPS_STACK_TOP, VM_PROT_READ | VM_PROT_WRITE);
  int error = vm_map_insert(p->p_uspace, stack_seg, VM_FIXED);
  assert(error == 0);

  klog("Stack real top at %p", (void *)MIPS_STACK_TOP);

  vm_map_activate(p->p_uspace);
}

/* Return to the previous map, unmodified by exec. */
static void restore_vmspace(proc_t *p, exec_vmspace_t *saved) {
  p->p_uspace = saved->uspace;
  p->p_sbrk = saved->sbrk;
  p->p_sbrk_end = saved->sbrk_end;
  vm_map_activate(p->p_uspace);
}

/* Destroy the vm_map we began preparing. */
static void destroy_vmspace(exec_vmspace_t *saved) {
  vm_map_delete(saved->uspace);
}

/* XXX We assume process may only have a single thread. But if there were more
 * than one thread in the process that called exec, all other threads must be
 * forcefully terminated. */
int do_exec(const exec_args_t *args) {
  thread_t *td = thread_self();
  proc_t *p = td->td_proc;
  vnode_t *vn;
  vattr_t attr;
  int error;

  assert(p != NULL);

  if ((error = open_executable(args->prog_name, &vn, &attr)))
    return error;

  Elf32_Ehdr eh;
  if ((error = check_elf(vn, &attr, &eh)))
    return error;

  /* We can not destroy the current vm map, because exec can still fail.
   * Is such case we must be able to return to the original address space. */
  exec_vmspace_t saved;
  enter_new_vmspace(p, &saved);

  if ((error = load_elf(p, vn, &eh)))
    goto fail;

  /* Prepare program stack, which includes storing program args. */
  vaddr_t stack_top = MIPS_STACK_TOP;
  if ((error = exec_args_copyout(args, &stack_top)))
    goto fail;

  /* Set up user context. */
  exc_frame_init(td->td_uframe, (void *)eh.e_entry, (void *)stack_top, EF_USER);

  /* At this point we are certain that exec succeeds.  We can safely destroy the
   * previous vm_map, and permanently assign this one to the current process. */
  destroy_vmspace(&saved);

  vm_map_dump(p->p_uspace);

  klog("Enter userspace with: pc=%p, sp=%p", eh.e_entry, stack_top);
  return -EJUSTRETURN;

fail:
  restore_vmspace(p, &saved);
  destroy_vmspace(&saved);
  return error;
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
