#define KL_LOG KL_PROC
#include <sys/klog.h>
#define _EXEC_IMPL
#include <sys/exec.h>
#include <sys/libkern.h>
#include <sys/vm_map.h>
#include <sys/vm_object.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/proc.h>

int exec_elf_inspect(vnode_t *vn, Elf_Ehdr *eh) {
  int error;
  vattr_t attr;

  if ((error = VOP_GETATTR(vn, &attr)))
    return error;

  if (attr.va_size < sizeof(Elf_Ehdr)) {
    klog("Exec failed: ELF file is too small to contain a valid header");
    return ENOEXEC;
  }

  klog("User ELF size: %u", attr.va_size);
  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 0, eh, sizeof(Elf_Ehdr));
  if ((error = VOP_READ(vn, &uio, 0))) {
    klog("Exec failed: Reading ELF header failed.");
    return error;
  }
  assert(uio.uio_resid == 0);

  /* Start by determining the validity of the elf file. */

  /* First, check for the magic header. */
  if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1 ||
      eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3) {
    klog("Exec failed: Incorrect ELF magic number");
    return ENOEXEC;
  }
  /* Check ELF class */
  if (eh->e_ident[EI_CLASS] != ELFCLASS) {
    klog("Exec failed: Unsupported ELF class");
    return ENOEXEC;
  }
  /* Check data format endianess */
  if (eh->e_ident[EI_DATA] != ELFDATA2LSB) {
    klog("Exec failed: ELF file is not low-endian");
    return ENOEXEC;
  }
  /* Ignore version and os abi field */
  /* Check file type */
  if (eh->e_type != ET_EXEC) {
    klog("Exec failed: ELF is not an executable file");
    return ENOEXEC;
  }
  /* Check machine architecture field */
  if (eh->e_machine != EM_ARCH) {
    klog("Exec failed: ELF target architecture is not supported");
    return ENOEXEC;
  }
  /* Check ELF header size */
  if (eh->e_ehsize != sizeof(Elf_Ehdr)) {
    klog("Exec failed: incorrect ELF header size");
    return ENOEXEC;
  }
  /* Check prog header size */
  if (eh->e_phentsize != sizeof(Elf_Phdr)) {
    klog("Exec failed: incorrect ELF program header size");
    return ENOEXEC;
  }
  /* Check number of prog headers */
  if (eh->e_phnum >= ELF_MAXPHNUM) {
    klog("Exec failed: too many program headers");
    return ENOEXEC;
  }
  /* Check section entry header size */
  if (eh->e_shentsize != sizeof(Elf_Shdr)) {
    klog("Exec failed: incorrect ELF section header size");
    return ENOEXEC;
  }

  return 0;
}

static int load_elf_segment(proc_t *p, vnode_t *vn, Elf_Phdr *ph) {
  int error;

  /* Avoid creating empty vm_map entries for segments that occupy no space in
   * memory, as they might overlap with subsequent segments. */
  if (ph->p_memsz == 0)
    return 0;

  klog("PT_LOAD: VAddr %08x Offset %08x FileSz %08x MemSz %08x Flags %d",
       (void *)ph->p_vaddr, (unsigned)ph->p_offset, (unsigned)ph->p_filesz,
       (unsigned)ph->p_memsz, (unsigned)ph->p_flags);

  if (!page_aligned_p(ph->p_vaddr)) {
    klog("Exec failed: Segment virtual address is not page aligned!");
    return ENOEXEC;
  }

  vaddr_t start = ph->p_vaddr;
  vaddr_t end = roundup(ph->p_vaddr + ph->p_memsz, PAGESIZE);

  /* Temporarily permissive protection. */
  vm_object_t *obj = vm_object_alloc(VM_ANONYMOUS);
  vm_segment_t *seg = vm_segment_alloc(
    obj, start, end, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC,
    VM_SEG_PRIVATE);
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
      UIO_SINGLE_USER(UIO_READ, ph->p_offset, (char *)start, ph->p_filesz);
    if ((error = VOP_READ(vn, &uio, 0))) {
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

int exec_elf_load(proc_t *p, vnode_t *vn, Elf_Ehdr *eh) {
  /* We know that ELF header was verified in inspect function. */
  int error;

  /* Read in program headers. */
  size_t phs_size = eh->e_phnum * eh->e_phentsize;
  char *phs = kmalloc(M_TEMP, phs_size, 0);
  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, eh->e_phoff, phs, phs_size);
  if ((error = VOP_READ(vn, &uio, 0))) {
    klog("Exec failed: Reading program headers failed.");
    goto fail;
  }
  assert(uio.uio_resid == 0);

  /* Iterate over program headers */
  klog("ELF has %d program headers", eh->e_phnum);
  for (int i = 0; i < eh->e_phnum; i++) {
    Elf_Phdr *ph = (Elf_Phdr *)(phs + i * eh->e_phentsize);
    error = ENOEXEC; /* default fail reason */
    switch (ph->p_type) {
      case PT_LOAD:
        if ((error = load_elf_segment(p, vn, ph)))
          goto fail;
        break;
      case PT_DYNAMIC:
      case PT_INTERP:
        klog("Exec failed: ELF file requests dynamic linking"
             "by providing a PT_DYNAMIC and/or PT_INTERP segment");
        goto fail;
      case PT_SHLIB:
        klog("Exec failed: ELF file contains a PT_SHLIB segment");
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
