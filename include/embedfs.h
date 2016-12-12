#ifndef _EMBEDFS_H_
#define _EMBEDFS_H_

#include <common.h>
#include <queue.h>
#include <linker_set.h>

#define EMBEDFS_NAME_MAX 64

typedef struct vnode vnode_t;

/* Do not create instances of this structure. Use EMBED_FILE instead. */
typedef struct embedfs_entry {
  char name[EMBEDFS_NAME_MAX];
  uint8_t *start;
  size_t size;
  vnode_t *vnode;
  TAILQ_ENTRY(embedfs_entry) list;
} embedfs_entry_t;

/* This macro marks a new file to be provided by embedfs. You may use it in any
 * translation unit to have a new file available through /embed. Remember that
 * creating an entry is one thing, you still need to tell the linker to actually
 * place the file in kernel image (see LD_EMBED in Makefile.common)*/
#define EMBED_FILE(_name, binname)                                             \
  extern uint8_t _binary_##binname##_start[];                                  \
  extern uint8_t _binary_##binname##_size[];                                   \
  embedfs_entry_t embedfs_entry_##binname = {                                  \
    .name = _name,                                                             \
    .start = _binary_##binname##_start,                                        \
    .size = (size_t)_binary_##binname##_size,                                  \
  };                                                                           \
  SET_ENTRY(embedfs_entries, embedfs_entry_##binname);

#endif /* !_EMBEDFS_H_ */
