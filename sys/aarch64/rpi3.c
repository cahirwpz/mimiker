#include <sys/kenv.h>
#include <sys/libkern.h>
#include <sys/thread.h>
#include <sys/types.h>
#include <aarch64/atags.h>
#include <aarch64/exception.h>
#include <aarch64/vm_param.h>

#define ENVBUF_SIZE 64

static char _memsize[ENVBUF_SIZE] = "memsize=";
static char _rd_start[ENVBUF_SIZE] = "rd_start=";
static char _rd_size[ENVBUF_SIZE] = "rd_size=";

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

static void itoa(uint32_t val, char *dest) {
  uint32_t tmp;
  char *c = dest;
  if (val == 0) {
    *c = '0';
    return;
  }
  while (val) {
    *c++ = '0' + (val % 10);
    val /= 10;
  }
  c--;
  while (dest < c) {
    tmp = *dest;
    *dest++ = *c;
    *c-- = tmp;
  }
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
        itoa(tag->u.tag_mem.size, &_memsize[0] + strlen(_memsize));
        *tokens++ = &_memsize[0];
        break;
      case ATAG_INITRD2:
        itoa(tag->u.tag_initrd.start, &_rd_start[0] + strlen(_rd_start));
        itoa(tag->u.tag_initrd.size, &_rd_size[0] + strlen(_rd_size));
        *tokens++ = &_rd_start[0];
        *tokens++ = &_rd_size[0];
        break;
      case ATAG_CMDLINE:
        *tokens++ = &tag->u.tag_cmd.command[0];
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
