#include <common.h>
#include <stdc.h>
#include <stack.h>
#include <exec.h>
#include <systm.h>
#include <errno.h>


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
void stack_user_entry_setup(const exec_args_t *args,
                            vm_addr_t *stack_bottom_p) {
  vm_addr_t stack_end = *stack_bottom_p;
  /* Begin by calculting arguments total size. This has to be done */
  /* in advance, because stack grows downwards. */
  size_t total_arg_size = 0;
  for (size_t i = 0; i < args->argc; i++) {
    total_arg_size += roundup(strlen(args->argv[i]) + 1, 4);
  }
  /* Store arguments, creating the argument vector. */
  vm_addr_t arg_vector[args->argc];
  vm_addr_t p = *stack_bottom_p - total_arg_size;
  for (unsigned i = 0; i < args->argc; i++) {
    size_t n = strlen(args->argv[i]) + 1;
    arg_vector[i] = p;
    memcpy((uint8_t *)p, args->argv[i], n);
    p += n;
    /* Align to word size */
    p = align(p, 4);
  }
  assert(p == stack_end);

  /* Move the stack down and align it so that the top of the stack will be
     8-byte alligned. */
  *stack_bottom_p = (*stack_bottom_p - total_arg_size) & 0xfffffff8;
  if (args->argc % 2 == 0)
    *stack_bottom_p -= 4;

  /* Now, place the argument vector on the stack. */
  size_t arg_vector_size = sizeof(arg_vector);
  vm_addr_t argv = *stack_bottom_p - arg_vector_size;
  memcpy((void *)argv, &arg_vector, arg_vector_size);
  /* Move the stack down. */
  *stack_bottom_p = *stack_bottom_p - arg_vector_size;

  /* Finally, place argc on the stack */
  *stack_bottom_p = *stack_bottom_p - sizeof(vm_addr_t);
  vm_addr_t *stack_args = (vm_addr_t *)*stack_bottom_p;
  *stack_args = args->argc;

  /* Top of the stack must be 8-byte alligned. */
  assert((*stack_bottom_p & 0x7) == 0);

  /* TODO: Environment */
}

void patch_blob(void *blob, size_t addr, bool add) {

  size_t argc = ((size_t *) blob)[0];
  char **argv = (char **) ( blob + sizeof(argc) );


  size_t oldbase = (size_t)(blob);

  
  for(size_t i = 0; i < argc; i++) {

    argv[i] = argv[i] + (addr - oldbase);

  }


}


void stack_user_entry_setup_proper(const exec_args_t_proper *args,
                            vm_addr_t *stack_bottom_p) {


  size_t total_arg_size = args->written;

  assert((total_arg_size % 8) == 0);
  assert((*stack_bottom_p % 8) == 0);
  *stack_bottom_p = (*stack_bottom_p - total_arg_size);
  patch_blob(args->blob, (size_t)(*stack_bottom_p), true);
  memcpy((uint8_t *)*stack_bottom_p, args->blob, total_arg_size);  
  

  /* //vm_addr_t stack_end = *stack_bottom_p; */
  /* size_t total_arg_size = args->written; */
  /* total_arg_size = roundup(total_arg_size, 8); */
      
  /* size_t argc  = ((size_t *)args->blob)[0]; */
  /* char **argsargv = (char**)(args->blob+ sizeof(argc)); */

  /* /// kprintf("@@@@argc is %d\n", argc); */
  /* //kprintf("@@@argv[0] is %s\n", argsargv[0]); */
  /* //kprintf("@@@argv[1] is %s\n", argsargv[1]); */
  
  
  /* /\* /\\* Begin by calculting arguments total size. This has to be done *\\/ *\/ */
  /* /\* /\\* in advance, because stack grows downwards. *\\/ *\/ */
  /* /\* size_t total_arg_size = 0; *\/ */
  /* /\* for (size_t i = 0; i < argc; i++) { *\/ */
  /* /\*   total_arg_size += roundup(strlen(argsargv[i]) + 1, 4); *\/ */
  /* /\* } *\/ */
  /* /\* /\\* Store arguments, creating the argument vector. *\\/ *\/ */
  /* /\* vm_addr_t arg_vector[argc]; *\/ */
  /* vm_addr_t p = *stack_bottom_p - total_arg_size; */


  /* kprintf("@@@@argc is %d\n", argc); */
  /* kprintf("@@@argv[0] is %s\n", argsargv[0]) ; */

  
  /* patch_blob(args->blob, (size_t)p, true); */

  /* memcpy((uint8_t *)p, args->blob, total_arg_size); */

  /* // kprintf("@@@@argc is %d\n", ((size_t *)p)[0]); */

  /*   size_t pargc  = ((size_t *)p)[0]; */
  /*   char **pargv = ((char**)(((uint8_t *)p) + sizeof(size_t))); */
  
  /*  kprintf("@@@pargc is %d\n",   pargc) ; */
  /*  kprintf("@@@pargv[0] is %s\n",   pargv[0]) ; */

  /* /\* Top of the stack must be 8-byte alligned. *\/ */
  /* assert((*stack_bottom_p & 0x7) == 0); */

   
  
  /* //kprintf("@@@argv[1] is %s\n", argsargv[1]); */

  
  /* //patching */

     
  /* vm_addr_t stack_end = *stack_bottom_p; */

  /* size_t argc  = ((size_t *)args->blob)[0]; */
  /* char **argsargv = (char**)(args->blob+ sizeof(argc)); */
  
  /* /\* Begin by calculting arguments total size. This has to be done *\/ */
  /* /\* in advance, because stack grows downwards. *\/ */
  /* size_t total_arg_size = 0; */
  /* for (size_t i = 0; i < argc; i++) { */
  /*   total_arg_size += roundup(strlen(argsargv[i]) + 1, 4); */
  /* } */
  /* /\* Store arguments, creating the argument vector. *\/ */
  /* vm_addr_t arg_vector[argc]; */
  /* vm_addr_t p = *stack_bottom_p - total_arg_size; */
  /* for (unsigned i = 0; i < argc; i++) { */
  /*   size_t n = strlen(argsargv[i]) + 1; */
  /*   arg_vector[i] = p; */
  /*   memcpy((uint8_t *)p, argsargv[i], n); */
  /*   p += n; */
  /*   /\* Align to word size *\/ */
  /*   p = align(p, 4); */
  /* } */
  /* assert(p == stack_end); */
  /* /\* Move the stack down and align it so that the top of the stack will be */
  /*    8-byte alligned. *\/ */
  /* *stack_bottom_p = (*stack_bottom_p - total_arg_size) & 0xfffffff8; */
  /* if (argc % 2 == 0) */
  /*   *stack_bottom_p -= 4; */

  /* /\* Now, place the argument vector on the stack. *\/ */
  /* size_t arg_vector_size = sizeof(arg_vector); */
  /* vm_addr_t argv = *stack_bottom_p - arg_vector_size; */
  /* memcpy((void *)argv, &arg_vector, arg_vector_size); */
  /* /\* Move the stack down. *\/ */
  /* *stack_bottom_p = *stack_bottom_p - arg_vector_size; */

  /* /\* Finally, place argc on the stack *\/ */
  /* *stack_bottom_p = *stack_bottom_p - sizeof(vm_addr_t); */
  /* vm_addr_t *stack_args = (vm_addr_t *)*stack_bottom_p; */
  /* *stack_args = argc; */

  /* /\* Top of the stack must be 8-byte alligned. *\/ */
  /* assert((*stack_bottom_p & 0x7) == 0); */

  /* /\* TODO: Environment *\/ */


  
  /*   vm_addr_t stack_end = *stack_bottom_p; */

  /* size_t argc  = ((size_t *)args->blob)[0]; */
  /* char **argsargv = (char**)(args->blob+ sizeof(argc)); */
  
  /* /\* Begin by calculting arguments total size. This has to be done *\/ */
  /* /\* in advance, because stack grows downwards. *\/ */
  /* size_t total_arg_size = 0; */
  /* for (size_t i = 0; i < argc; i++) { */
  /*   total_arg_size += roundup(strlen(argsargv[i]) + 1, 4); */
  /* } */
  /* /\* Store arguments, creating the argument vector. *\/ */
  /* vm_addr_t arg_vector[argc]; */
  /* vm_addr_t p = *stack_bottom_p - total_arg_size; */
  /* for (unsigned i = 0; i < argc; i++) { */
  /*   size_t n = strlen(argsargv[i]) + 1; */
  /*   arg_vector[i] = p; */
  /*   memcpy((uint8_t *)p, argsargv[i], n); */
  /*   p += n; */
  /*   /\* Align to word size *\/ */
  /*   p = align(p, 4); */
  /* } */
  /* assert(p == stack_end); */
  /* /\* Move the stack down and align it so that the top of the stack will be */
  /*    8-byte alligned. *\/ */
  /* *stack_bottom_p = (*stack_bottom_p - total_arg_size) & 0xfffffff8; */
  /* if (argc % 2 == 0) */
  /*   *stack_bottom_p -= 4; */

  /* /\* Now, place the argument vector on the stack. *\/ */
  /* size_t arg_vector_size = sizeof(arg_vector); */
  /* vm_addr_t argv = *stack_bottom_p - arg_vector_size; */
  /* memcpy((void *)argv, &arg_vector, arg_vector_size); */
  /* /\* Move the stack down. *\/ */
  /* *stack_bottom_p = *stack_bottom_p - arg_vector_size; */

  /* /\* Finally, place argc on the stack *\/ */
  /* *stack_bottom_p = *stack_bottom_p - sizeof(vm_addr_t); */
  /* vm_addr_t *stack_args = (vm_addr_t *)*stack_bottom_p; */
  /* *stack_args = argc; */

  /* /\* Top of the stack must be 8-byte alligned. *\/ */
  /* assert((*stack_bottom_p & 0x7) == 0); */

  /* /\* TODO: Environment *\/ */

  /* assert( stack_end > *stack_bottom_p + args->written); */

  /* vm_addr_t stack_end = *stack_bottom_p; */

  /* size_t argc  = ((size_t *)args->blob)[0]; */
  /* char **argsargv = (char**)(args->blob+ sizeof(argc)); */
  
  /* /\* Begin by calculting arguments total size. This has to be done *\/ */
  /* /\* in advance, because stack grows downwards. *\/ */
  /* size_t total_arg_size = 0; */
  /* for (size_t i = 0; i < argc; i++) { */
  /*   total_arg_size += roundup(strlen(argsargv[i]) + 1, 4); */
  /* } */
  /* /\* Store arguments, creating the argument vector. *\/ */
  /* vm_addr_t arg_vector[argc]; */
  /* vm_addr_t p = *stack_bottom_p - total_arg_size; */
  /* for (unsigned i = 0; i < argc; i++) { */
  /*   size_t n = strlen(argsargv[i]) + 1; */
  /*   arg_vector[i] = p; */
  /*   memcpy((uint8_t *)p, argsargv[i], n); */
  /*   p += n; */
  /*   /\* Align to word size *\/ */
  /*   p = align(p, 4); */
  /* } */
  /* assert(p == stack_end); */
  /* /\* Move the stack down and align it so that the top of the stack will be */
  /*    8-byte alligned. *\/ */
  /* *stack_bottom_p = (*stack_bottom_p - total_arg_size) & 0xfffffff8; */
  /* if (argc % 2 == 0) */
  /*   *stack_bottom_p -= 4; */

  /* /\* Now, place the argument vector on the stack. *\/ */
  /* size_t arg_vector_size = sizeof(arg_vector); */
  /* vm_addr_t argv = *stack_bottom_p - arg_vector_size; */
  /* memcpy((void *)argv, &arg_vector, arg_vector_size); */
  /* /\* Move the stack down. *\/ */
  /* *stack_bottom_p = *stack_bottom_p - arg_vector_size; */

  /* /\* Finally, place argc on the stack *\/ */
  /* *stack_bottom_p = *stack_bottom_p - sizeof(vm_addr_t); */
  /* vm_addr_t *stack_args = (vm_addr_t *)*stack_bottom_p; */
  /* *stack_args = argc; */

  /* /\* Top of the stack must be 8-byte alligned. *\/ */
  /* assert((*stack_bottom_p & 0x7) == 0); */

  /* /\* TODO: Environment *\/ */



  /* vm_addr_t stack_end = *stack_bottom_p; */
  /* /\* Begin by calculting arguments total size. This has to be done *\/ */
  /* /\* in advance, because stack grows downwards. *\/ */
  /* size_t total_arg_size = 0; */
  /* for (size_t i = 0; i < args->argc; i++) { */
  /*   total_arg_size += roundup(strlen(args->argv[i]) + 1, 4); */
  /* } */
  /* /\* Store arguments, creating the argument vector. *\/ */
  /* vm_addr_t arg_vector[args->argc]; */
  /* vm_addr_t p = *stack_bottom_p - total_arg_size; */
  /* for (unsigned i = 0; i < args->argc; i++) { */
  /*   size_t n = strlen(args->argv[i]) + 1; */
  /*   arg_vector[i] = p; */
  /*   memcpy((uint8_t *)p, args->argv[i], n); */
  /*   p += n; */
  /*   /\* Align to word size *\/ */
  /*   p = align(p, 4); */
  /* } */
  /* assert(p == stack_end); */

  /* /\* Move the stack down and align it so that the top of the stack will be */
  /*    8-byte alligned. *\/ */
  /* *stack_bottom_p = (*stack_bottom_p - total_arg_size) & 0xfffffff8; */
  /* if (args->argc % 2 == 0) */
  /*   *stack_bottom_p -= 4; */

  /* /\* Now, place the argument vector on the stack. *\/ */
  /* size_t arg_vector_size = sizeof(arg_vector); */
  /* vm_addr_t argv = *stack_bottom_p - arg_vector_size; */
  /* memcpy((void *)argv, &arg_vector, arg_vector_size); */
  /* /\* Move the stack down. *\/ */
  /* *stack_bottom_p = *stack_bottom_p - arg_vector_size; */

  /* /\* Finally, place argc on the stack *\/ */
  /* *stack_bottom_p = *stack_bottom_p - sizeof(vm_addr_t); */
  /* vm_addr_t *stack_args = (vm_addr_t *)*stack_bottom_p; */
  /* *stack_args = args->argc; */

  /* /\* Top of the stack must be 8-byte alligned. *\/ */
  /* assert((*stack_bottom_p & 0x7) == 0); */

  /* /\* TODO: Environment *\/ */
}


typedef struct copy_ops
{
  int (*cpybytes)(const void* saddr, void* daddr, size_t len);
  int (*cpystr)(const void* saddr, void* daddr, size_t len, size_t* lencopied);
} copy_ops_t;

copy_ops_t from_userspace = 
{
  .cpybytes = copyin,
  .cpystr = copyinstr
};

int memcpy_wrapper(const void* saddr, void* daddr, size_t len) {

  memcpy(daddr, saddr, len);
  return 0;
}
int cpystr_wrapper(const void* saddr, void* daddr, size_t len, size_t* lencopied) {

  size_t actlen = strlen(saddr);
  int result = 0;

  if (actlen + 1 > len) {
    result = -ENAMETOOLONG; 
    actlen = len;
  }

  *lencopied = actlen; 
 memcpy(daddr, saddr, actlen);
 return result;
}

copy_ops_t from_kernelspace =  {

  .cpybytes = memcpy_wrapper,
  .cpystr = cpystr_wrapper  
};



int copyinargptrs_proper(const char **user_argv,
			 int8_t* blob, size_t blob_size,
			 size_t *argc_out, copy_ops_t co) {

  const char** kern_argv = (const char**)blob;

  char *arg_ptr;
  const size_t ptr_size = sizeof(arg_ptr);

  for (int argc = 0; argc * ptr_size < blob_size; argc++)
  {
    int result = co.cpybytes(user_argv + argc, &arg_ptr, ptr_size);
    if (result < 0)
      return result;

    kern_argv[argc] = arg_ptr;
    if (arg_ptr != NULL) // Do we need to copy more arg ptrs?
      continue;

    // We copied all arg ptrs :)
    if (argc == 0) // Is argument list empty?
      return -EFAULT;

    // Its not empty and we copied all arg ptrs - Success!
    *argc_out = argc;
    return 0;
  }

  // We execeeded ARG_MAX while copying arguments
  return -E2BIG;
}


int marshal_args(const char **user_argv, int8_t *argv_blob,
		 size_t blob_size, size_t *bytes_written, copy_ops_t co);


int uspace_marshal_args(const char **user_argv, int8_t *argv_blob,
		 size_t blob_size, size_t *bytes_written) {


  return marshal_args(user_argv, argv_blob, blob_size, bytes_written, from_userspace);
  
} 


int kspace_marshal_args(const char **user_argv, int8_t *argv_blob,
		 size_t blob_size, size_t *bytes_written) {


  return marshal_args(user_argv, argv_blob, blob_size, bytes_written, from_kernelspace);
  
} 


int marshal_args(const char **user_argv, int8_t *argv_blob,
		 size_t blob_size, size_t *bytes_written, copy_ops_t co) {
  
  assert(sizeof(vm_addr_t) == 4); 
  assert(sizeof(size_t) == 4);
  assert(sizeof(char*) == 4);
  assert( blob_size >= 4);

  size_t argc;
  char **kern_argv = (char**)(argv_blob + sizeof(argc));
    
  int result = copyinargptrs_proper( user_argv, (int8_t*)kern_argv,
				     blob_size - sizeof(argc),
				     &argc, co);   

  if (result < 0)
    return result;

  assert(argc > 0);
  
  *bytes_written = argc * sizeof(char *);


  ((size_t*)argv_blob)[0] = argc;
  *bytes_written += sizeof(argc);
  *bytes_written = roundup(*bytes_written, 8);

  
  int8_t *args  =  argv_blob + *bytes_written;
  size_t argsize, i = 0;
  
  do {

    result = co.cpystr(kern_argv[i], args,
		       blob_size - *bytes_written , &argsize);
    if (result < 0)
      return (result == -ENAMETOOLONG) ? -E2BIG : result;

    kern_argv[i] = (char*)(args);
    argsize = roundup(argsize, 4);
    args += argsize;
    *bytes_written += argsize;
    i++;

  } while ( (i < argc) && (*bytes_written < blob_size)); 

  if (i < argc)
    return -E2BIG;
    
  *bytes_written = roundup(*bytes_written, 8);
    
  return 0;
}
