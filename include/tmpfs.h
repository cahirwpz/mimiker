#include <vnode.h>
#include <queue.h>

struct tmpfs_node;

typedef struct tmpfs_dirnode_data {
  TAILQ_HEAD(, tmpfs_node) head;
} tmpfs_dirnode_data_t;

typedef struct tmpfs_node_data {
  char *buf;
  int size;
} tmpfs_node_data_t;

typedef enum { T_REG, T_DIR } tmpfs_node_type;

typedef struct tmpfs_node {
  const char *name;
  TAILQ_ENTRY(tmpfs_node) direntry;

  tmpfs_node_type type;
  union {
    tmpfs_node_data_t data;
    tmpfs_dirnode_data_t dirdata;
  };

} tmpfs_node_t;

extern vnodeops_t tmpfs_ops;
