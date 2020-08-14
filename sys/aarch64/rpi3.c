#include <sys/kenv.h>
#include <sys/libkern.h>
#include <sys/thread.h>
#include <sys/types.h>
#include <aarch64/atags.h>
#include <aarch64/exception.h>
#include <aarch64/vm_param.h>

#define ENVBUF_SIZE 64

static char _memsize[ENVBUF_SIZE];
static char _rd_start[ENVBUF_SIZE];
static char _rd_size[ENVBUF_SIZE];

static int count_tokens(atag_tag_t *atags) {
  int ntokens = 0;
  atag_tag_t *tag;
  ATAG_FOREACH(tag, atags) {
    switch (ATAG_TAG(tag)) {
      case ATAG_MEM:
        ntokens++;
        break;
      case ATAG_INITRD2:
        ntokens += 2;
        break;
      case ATAG_CMDLINE:
        ntokens++;
        break;
      default:
        break;
    }
  }
  return ntokens;
}

void *board_stack(atag_tag_t *atags) {
  kstack_t *stk = &thread0.td_kstack;

  thread0.td_uframe = kstack_alloc_s(stk, exc_frame_t);

  int ntokens = count_tokens(atags);

  char **kenvp = kstack_alloc(stk, (ntokens + 2) * sizeof(char *));
  char **kinit = NULL;

  char **tokens = kenvp;

  atag_tag_t *tag;
  ATAG_FOREACH(tag, atags) {
    switch (ATAG_TAG(tag)) {
      case ATAG_MEM:
        snprintf(_memsize, sizeof(_memsize), "memsize=%d",
                 tag->u.tag_mem.size);
        *tokens++ = _memsize;
        break;
      case ATAG_INITRD2:
        snprintf(_rd_start, sizeof(_rd_start), "rd_start=%d", 
                 tag->u.tag_initrd.start);
        snprintf(_rd_size, sizeof(_rd_size), "rd_size=%d",
                 tag->u.tag_initrd.size);
        *tokens++ = _rd_start;
        *tokens++ = _rd_size;
        break;
      case ATAG_CMDLINE:
        *tokens++ = tag->u.tag_cmd.command;
        break;
      default:
        break;
    }
  }

  *tokens = NULL;

  kstack_fix_bottom(stk);

  for (char **argp = kenvp; *argp; argp++) {
    if (strcmp("--", *argp) == 0) {
      *argp++ = NULL;
      kinit = argp;
      break;
    }
  }

  init_kenv(kenvp, kinit);

  return stk->stk_ptr;
}

intptr_t ramdisk_get_start(void) {
  return kenv_get_ulong("rd_start");
}

size_t ramdisk_get_size(void) {
  return align(kenv_get_ulong("rd_size"), PAGESIZE);
}
