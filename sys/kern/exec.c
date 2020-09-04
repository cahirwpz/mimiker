#define KL_LOG KL_PROC
#include <sys/klog.h>
#define _EXEC_IMPL
#include <sys/exec.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/vm_map.h>
#include <sys/vm_object.h>
#include <sys/thread.h>
#include <sys/errno.h>
#include <sys/filedesc.h>
#include <sys/sbrk.h>
#include <sys/syslimits.h>
#include <sys/vfs.h>
#include <sys/ustack.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/malloc.h>

typedef int (*copy_ptr_t)(exec_args_t *args, char *const *ptr_p);
typedef int (*copy_str_t)(exec_args_t *args, const char *str, size_t *copied_p);

/* Adds working buffers to exec_args structure. */
static void exec_args_init(exec_args_t *args) {
  args->path = kmalloc(M_TEMP, PATH_MAX, 0);
  args->data = kmalloc(M_TEMP, ARG_MAX, 0);
  args->end = args->data;
  args->left = ARG_MAX;
  args->interp = NULL;

  /* HACK Let's reserve one entry just before argv for interpreter path.
   * There's an implicit assumption that argv goes into the buffer first. */
  bzero(args->end, sizeof(char *));
  args->end += sizeof(char *);
  args->left -= sizeof(char *);
}

/* Frees dynamically allocated memory from exec_args structure */
static void exec_args_destroy(exec_args_t *args) {
  kfree(M_TEMP, args->path);
  kfree(M_TEMP, args->data);
}

/* Procedures for copying data coming from kernel space into exec_args buffer */
static int kern_copy_path(exec_args_t *args, const char *path) {
  return (strlcpy(args->path, path, PATH_MAX) >= PATH_MAX) ? ENAMETOOLONG : 0;
}

static int kern_copy_ptr(exec_args_t *args, char *const *src_p) {
  *(char **)args->end = *src_p;
  return 0;
}

static int kern_copy_str(exec_args_t *args, const char *str, size_t *copied_p) {
  size_t copied = strlcpy(args->end, str, args->left);
  *copied_p = copied + 1;
  return (copied == args->left) ? E2BIG : 0; /* no space for NUL ? */
}

/* Procedures for copying data coming from user space into exec_args buffer */
static int user_copy_path(exec_args_t *args, const char *path) {
  return copyinstr(path, args->path, PATH_MAX, NULL);
}

static int user_copy_ptr(exec_args_t *args, char *const *user_src_p) {
  return copyin(user_src_p, args->end, sizeof(char *));
}

static int user_copy_str(exec_args_t *args, const char *str, size_t *copied_p) {
  int error = copyinstr(str, args->end, args->left, copied_p);
  return (error == ENAMETOOLONG) ? E2BIG : error;
}

/* Copy argv/envv pointers into exec_args buffer */
static int copy_ptrs(exec_args_t *args, copy_ptr_t copy_ptr, char ***dstv_p,
                     size_t *len_p, char *const *srcv) {
  char **dstv = (char **)args->end;
  int error, n = 0;

  /* Copy pointers one by one. */
  do {
    if (args->left < sizeof(char *))
      return E2BIG;
    if ((error = copy_ptr(args, srcv + n)))
      return error;
    args->end += sizeof(char *);
    args->left -= sizeof(char *);
  } while (dstv[n++]);

  *dstv_p = dstv;
  *len_p = n - 1;
  return 0;
}

/* Copy argv/envv strings into exec_args buffer */
static int copy_strv(exec_args_t *args, copy_str_t copy_str, char **strv) {
  int error;

  for (int i = 0; strv[i]; i++) {
    size_t copied;
    if ((error = copy_str(args, strv[i], &copied)))
      return error;
    if (copied >= args->left)
      return E2BIG;
    strv[i] = args->end;
    args->end += copied;
    args->left -= copied;
  }

  return 0;
}

/* Copy argv/envv pointers and strings into exec_args buffer */
static int copy_args(exec_args_t *args, copy_ptr_t copy_ptr,
                     copy_str_t copy_str, char *const *argv,
                     char *const *envv) {
  int error;

  if ((error = copy_ptrs(args, copy_ptr, &args->argv, &args->argc, argv)) ||
      (error = copy_ptrs(args, copy_ptr, &args->envv, &args->envc, envv)) ||
      (error = copy_strv(args, copy_str, args->argv)) ||
      (error = copy_strv(args, copy_str, args->envv)))
    return error;

  return 0;
}

static int user_copy_args(exec_args_t *args, char *const *argv,
                          char *const *envv) {
  return copy_args(args, user_copy_ptr, user_copy_str, argv, envv);
}

static int kern_copy_args(exec_args_t *args, char *const *argv,
                          char *const *envv) {
  return copy_args(args, kern_copy_ptr, kern_copy_str, argv, envv);
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
static int exec_args_copyout(exec_args_t *args, vaddr_t *stack_top_p) {
  ustack_t us;
  char **argv, **envv;
  int error;
  size_t argc = args->argc, envc = args->envc;

  ustack_setup(&us, *stack_top_p, ARG_MAX);

  if ((error = ustack_push_int(&us, argc)) ||
      (error = ustack_alloc_ptr_n(&us, argc, (vaddr_t *)&argv)) ||
      (error = ustack_push_long(&us, (long)NULL)) ||
      (error = ustack_alloc_ptr_n(&us, envc, (vaddr_t *)&envv)) ||
      (error = ustack_push_long(&us, (long)NULL)) ||
      (error = store_strings(&us, args->argv, argv, argc)) ||
      (error = store_strings(&us, args->envv, envv, envc)))
    goto fail;

  ustack_finalize(&us);

  for (size_t i = 0; i < argc; i++)
    ustack_relocate_ptr(&us, (vaddr_t *)&argv[i]);
  for (size_t i = 0; i < envc; i++)
    ustack_relocate_ptr(&us, (vaddr_t *)&envv[i]);

  error = ustack_copy(&us, stack_top_p);

fail:
  ustack_teardown(&us);
  return error;
}

static int open_executable(const char *path, vnode_t **vn_p) {
  vnode_t *vn = *vn_p;
  int error;

  klog("Loading program '%s'", path);

  /* Translate program name to vnode. */
  if ((error = vfs_namelookup(path, &vn)))
    return error;

  /* It must be a regular executable file with non-zero size. */
  if (vn->v_type != V_REG)
    return EACCES;
  if ((error = VOP_ACCESS(vn, VEXEC)))
    return error;

  /* TODO Some checks are missing:
   * 1) Check if NOEXEC bit is set on the filesystem this file resides on.
   * 2) If file is opened for write then return ETXTBSY */

  *vn_p = vn;
  return 0;
}

typedef struct exec_vmspace {
  vm_map_t *uspace;
  vm_segment_t *sbrk;
  vaddr_t sbrk_end;
} exec_vmspace_t;

static void enter_new_vmspace(proc_t *p, exec_vmspace_t *saved,
                              vaddr_t *stack_top_p) {
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
  *stack_top_p = USER_STACK_TOP;

  vm_object_t *stack_obj = vm_object_alloc(VM_ANONYMOUS);
  vm_segment_t *stack_seg =
    vm_segment_alloc(stack_obj, USER_STACK_TOP - USER_STACK_SIZE,
                     USER_STACK_TOP, VM_PROT_READ | VM_PROT_WRITE);
  int error = vm_map_insert(p->p_uspace, stack_seg, VM_FIXED);
  assert(error == 0);

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
static int _do_execve(exec_args_t *args) {
  thread_t *td = thread_self();
  proc_t *p = td->td_proc;
  vnode_t *vn;
  int error;

  assert(p != NULL);

  bool use_interpreter = false;
  char *prog;
  for (;;) {
    prog = args->interp ? args->interp : args->path;

    if ((error = open_executable(prog, &vn))) {
      klog("No file found: '%s'!", prog);
      return error;
    }

    /* Interpreter file must not be interpreted!
     * Otherwise a user could create an infinite chain of interpreted files. */
    if (use_interpreter)
      break;

    /* If shebang signature is detected use interpreter to load the file. */
    if ((error = exec_shebang_inspect(vn)))
      break;

    if ((error = exec_shebang_load(vn, args)))
      return error;

    klog("Interpreter for '%s' is '%s'", prog, args->interp);
    use_interpreter = true;
  }

  Elf_Ehdr eh;
  if ((error = exec_elf_inspect(vn, &eh)))
    return error;

  /* We can not destroy the current vm_map, because exec can still fail.
   * Is such case we must be able to return to the original address space. */
  exec_vmspace_t saved;
  vaddr_t stack_top;
  enter_new_vmspace(p, &saved, &stack_top);

  if ((error = exec_elf_load(p, vn, &eh)))
    goto fail;

  /* Prepare program stack, which includes storing program args. */
  if ((error = exec_args_copyout(args, &stack_top)))
    goto fail;

  fdtab_onexec(p->p_fdtable);

  /* Set up user context. */
  ctx_init((ctx_t *)td->td_uctx, (void *)eh.e_entry, (void *)stack_top,
           EF_USER);

  /* At this point we are certain that exec succeeds.  We can safely destroy the
   * previous vm_map, and permanently assign this one to the current process. */
  destroy_vmspace(&saved);

  vm_map_dump(p->p_uspace);

  kfree(M_STR, p->p_elfpath);
  p->p_elfpath = kstrndup(M_STR, prog, PATH_MAX);

  klog("Enter userspace with: pc=%p, sp=%p", eh.e_entry, stack_top);
  return EJUSTRETURN;

fail:
  restore_vmspace(p, &saved);
  destroy_vmspace(&saved);
  return error;
}

int do_execve(const char *u_path, char *const *u_argp, char *const *u_envp) {
  int result;
  exec_args_t args;
  exec_args_init(&args);
  if ((result = user_copy_path(&args, u_path)) ||
      (result = user_copy_args(&args, u_argp, u_envp)))
    goto end;
  result = _do_execve(&args);
end:
  exec_args_destroy(&args);
  return result;
}

__noreturn void kern_execve(const char *path, char *const *argv,
                            char *const *envv) {
  proc_t *p = proc_self();

  klog("PID %d: Starting program '%s'", p->p_pid, path);

  exec_args_t args;
  exec_args_init(&args);

  if (kern_copy_path(&args, path) || kern_copy_args(&args, argv, envv) ||
      _do_execve(&args) != EJUSTRETURN)
    panic("Failed to start '%s' program.", path);

  exec_args_destroy(&args);
  user_exc_leave();
}
