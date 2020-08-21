#include <sys/kenv.h>
#include <sys/cmdline.h>
#include <sys/libkern.h>
#include <sys/thread.h>
#include <aarch64/atags.h>
#include <aarch64/exception.h>
#include <aarch64/vm_param.h>

static int count_atags(atag_tag_t *atags) {
  int ntokens = 0;
  atag_tag_t *atag;
  ATAG_FOREACH(atag, atags) {
    uint32_t tag = ATAG_TAG(atag);
    if (tag == ATAG_MEM) {
      ntokens++;
    } else if (tag == ATAG_INITRD2) {
      ntokens += 2;
    } else if (tag == ATAG_CMDLINE) {
      ntokens += cmdline_count_tokens(atag->tag_cmd.command);
    }
  }
  return ntokens;
}

static void process_atags(atag_tag_t *atags, char **tokens, kstack_t *stk) {
  char buf[32];

  atag_tag_t *atag;
  ATAG_FOREACH(atag, atags) {
    uint32_t tag = ATAG_TAG(atag);
    if (tag == ATAG_MEM) {
      snprintf(buf, sizeof(buf), "memsize=%d", atag->tag_mem.size);
      tokens = cmdline_extract_tokens(stk, buf, tokens);
    } else if (tag == ATAG_INITRD2) {
      snprintf(buf, sizeof(buf), "rd_start=%d", atag->tag_initrd.start);
      tokens = cmdline_extract_tokens(stk, buf, tokens);
      snprintf(buf, sizeof(buf), "rd_size=%d", atag->tag_initrd.size);
      tokens = cmdline_extract_tokens(stk, buf, tokens);
    } else if (tag == ATAG_CMDLINE) {
      tokens = cmdline_extract_tokens(stk, atag->tag_cmd.command, tokens);
    }
  }

  *tokens = NULL;
}

void *board_stack(atag_tag_t *atags) {
  kstack_t *stk = &thread0.td_kstack;

  thread0.td_uframe = kstack_alloc_s(stk, exc_frame_t);

  int ntokens = count_atags(atags);
  char **kenvp = kstack_alloc(stk, (ntokens + 2) * sizeof(char *));
  process_atags(atags, kenvp, stk);
  kstack_fix_bottom(stk);
  init_kenv(kenvp);

  return stk->stk_ptr;
}

intptr_t ramdisk_get_start(void) {
  return kenv_get_ulong("rd_start");
}

size_t ramdisk_get_size(void) {
  return align(kenv_get_ulong("rd_size"), PAGESIZE);
}
