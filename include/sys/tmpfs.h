#ifndef _SYS_TMPFS_H_
#define _SYS_TMPFS_H_

#include <sys/queue.h>
#include <sys/vnode.h>

#define TMPFS_NAME_MAX 64

typedef struct tmpfs_dirent {
  TAILQ_ENTRY(tmpfs_dirent) td_entries;
  /* Pointer to the inode this entry refers to. */
  struct tmpfs_node *td_node;

  /* Name and its length. */
  char *td_name;
  size_t td_namelen;
} tmpfs_dirent_t;

typedef TAILQ_HEAD(, tmpfs_dirent) tmpfs_dirent_list_t;

typedef struct tmpfs_node {
  TAILQ_ENTRY(tmpfs_node) tn_entries;

  /* Pointer to the corrensponding vnode. */
  vnode_t *tn_vnode;
  /* Vnode type. */
  vnodetype_t tn_type;

  /* Number of file hard links. */
  nlink_t tn_links;

  /* Data that is only applicable to a particular type. */
  union {
    /* V_DIR */
    struct {
      /* List of directory entries. */
      tmpfs_dirent_list_t tn_dir;
    } tn_dir;

    /* V_REG */
    struct {

    } tn_reg;
  } tn_spec;

} tmpfs_node_t;

typedef struct tmpfs_mount {
  tmpfs_node_t *tm_root;
  mtx_t tm_lock;
} tmpfs_mount_t;

#endif /* !_SYS_TMPFS_H_ */
