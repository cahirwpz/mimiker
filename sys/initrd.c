#include <errno.h>
#include <common.h>
#include <malloc.h>
#include <vm.h>
#include <vm_map.h>
#include <stdc.h>
#include <cpio.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>
#include <mount.h>
#include <linker_set.h>

/* ramdisk related data that will be stored in v_data field of vnode */
typedef struct cpio_node {
  TAILQ_ENTRY(cpio_node) c_list; /* link to global list of all ramdisk nodes */
  TAILQ_HEAD(, cpio_node) c_children; /* head of list of direct descendants */
  TAILQ_ENTRY(cpio_node) c_siblings;  /* nodes that have the same parent */

  dev_t c_dev;
  ino_t c_ino;
  mode_t c_mode;
  nlink_t c_nlink;
  uid_t c_uid;
  gid_t c_gid;
  dev_t c_rdev;
  off_t c_size;
  time_t c_mtime;

  const char *c_path; /* contains exact path to file as archived by cpio */
  const char *c_name; /* contains name of file */
  void *c_data;
} cpio_node_t;

static MALLOC_DEFINE(mp, "initial ramdisk memory pool");
typedef TAILQ_HEAD(, cpio_node) cpio_list_t;

static cpio_list_t initrd_head;
static cpio_node_t *root_node;
static vnodeops_t initrd_ops = {.v_lookup = vnode_op_notsup,
                                .v_readdir = vnode_op_notsup,
                                .v_open = vnode_op_notsup,
                                .v_read = vnode_op_notsup,
                                .v_write = vnode_op_notsup};

extern char *kenv_get(const char *key);

static cpio_node_t *cpio_node_alloc() {
  cpio_node_t *node = kmalloc(mp, sizeof(cpio_node_t), M_ZERO);
  TAILQ_INIT(&node->c_children);
  return node;
}

static void cpio_node_dump(cpio_node_t *cn) {
  log("entry '%s': {dev: %ld, ino: %lu, mode: %d, nlink: %d, "
      "uid: %d, gid: %d, rdev: %ld, size: %ld, mtime: %lu}",
      cn->c_path, cn->c_dev, cn->c_ino, cn->c_mode, cn->c_nlink, cn->c_uid,
      cn->c_gid, cn->c_rdev, cn->c_size, cn->c_mtime);
}

static void read_bytes(void **tape, void *ptr, size_t bytes) {
  memcpy(ptr, *tape, bytes);
  *tape += bytes;
}

static void skip_bytes(void **tape, size_t bytes) {
  *tape = align(*tape + bytes, 4);
}

#define MKDEV(major, minor) (((major & 0xff) << 8) | (minor & 0xff))

static bool read_cpio_header(void **tape, cpio_node_t *cpio) {
  cpio_new_hdr_t hdr;

  read_bytes(tape, &hdr, sizeof(hdr));

  uint16_t c_magic = strntoul(hdr.c_magic, 6, NULL, 8);

  if (c_magic != CPIO_NMAGIC && c_magic != CPIO_NCMAGIC) {
    log("wrong magic number: %o", c_magic);
    return false;
  }

  uint32_t c_ino = strntoul(hdr.c_ino, 8, NULL, 16);
  uint32_t c_mode = strntoul(hdr.c_mode, 8, NULL, 16);
  uint32_t c_uid = strntoul(hdr.c_uid, 8, NULL, 16);
  uint32_t c_gid = strntoul(hdr.c_gid, 8, NULL, 16);
  uint32_t c_nlink = strntoul(hdr.c_nlink, 8, NULL, 16);
  uint32_t c_mtime = strntoul(hdr.c_mtime, 8, NULL, 16);
  uint32_t c_filesize = strntoul(hdr.c_filesize, 8, NULL, 16);
  uint32_t c_maj = strntoul(hdr.c_maj, 8, NULL, 16);
  uint32_t c_min = strntoul(hdr.c_min, 8, NULL, 16);
  uint32_t c_rmaj = strntoul(hdr.c_rmaj, 8, NULL, 16);
  uint32_t c_rmin = strntoul(hdr.c_rmin, 8, NULL, 16);
  uint32_t c_namesize = strntoul(hdr.c_namesize, 8, NULL, 16);

  cpio->c_dev = MKDEV(c_maj, c_min);
  cpio->c_ino = c_ino;
  cpio->c_mode = c_mode;
  cpio->c_nlink = c_nlink;
  cpio->c_uid = c_uid;
  cpio->c_gid = c_gid;
  cpio->c_rdev = MKDEV(c_rmaj, c_rmin);
  cpio->c_size = c_filesize;
  cpio->c_mtime = c_mtime;

  cpio->c_path = *tape;
  skip_bytes(tape, c_namesize);
  cpio->c_data = *tape;
  skip_bytes(tape, c_filesize);

  return true;
}

static void read_cpio_archive() {
  void *tape = (void *)ramdisk_get_start();

  while (true) {
    cpio_node_t *node = cpio_node_alloc();
    if (!read_cpio_header(&tape, node) ||
        strcmp(node->c_path, CPIO_TRAILER) == 0) {
      kfree(mp, node);
      break;
    }
    TAILQ_INSERT_TAIL(&initrd_head, node, c_list);
  }
}

/* Check if `p1` is a path to file/directory which can be contained directly
 * inside `p2` directory. */
static bool is_direct_descendant(const char *p1, const char *p2) {
  while (*p1 && *p1 == *p2) {
    p1++, p2++;
  }

  if (*p2)
    return false;

  /* Check whether p is valid filename (does not contain '/') */
  p1++; /* skip trailing '/' */
  return (strchr(p1, '/') == NULL);
}

/* Extract last component of the path. */
static const char *basename(const char *path) {
  char *name = strrchr(path, '/');
  return name ? name + 1 : path;
}

static void initrd_build_tree_and_names() {
  cpio_node_t *it_i, *it_j;

  TAILQ_FOREACH (it_i, &initrd_head, c_list) {
    TAILQ_FOREACH (it_j, &initrd_head, c_list) {
      if (it_i == it_j)
        continue;
      if (is_direct_descendant(it_j->c_path, it_i->c_path))
        TAILQ_INSERT_TAIL(&it_i->c_children, it_j, c_siblings);
    }
  }

  cpio_node_t *it;
  TAILQ_FOREACH (it, &initrd_head, c_list) {
    it->c_name = basename(it->c_path);
  }

  /* Construct a node that represent the root of filesystem. */
  root_node = cpio_node_alloc();
  root_node->c_path = "";
  TAILQ_FOREACH (it, &initrd_head, c_list) {
    if (is_direct_descendant(it->c_path, "")) {
      TAILQ_INSERT_TAIL(&root_node->c_children, it, c_siblings);
    }
  }
}

static int initrd_vnode_lookup(vnode_t *vdir, const char *name, vnode_t **res) {
  cpio_node_t *it;
  cpio_node_t *cn_dir = (cpio_node_t *)vdir->v_data;

  TAILQ_FOREACH (it, &cn_dir->c_children, c_siblings) {
    if (strcmp(name, it->c_name) == 0) {
      vnodetype_t type = V_REG;
      if (it->c_mode & C_ISDIR)
        type = V_DIR;
      *res = vnode_new(type, &initrd_ops);
      (*res)->v_data = (void *)it;
      vnode_ref(*res);
      return 0;
    }
  }
  return ENOENT;
}

static int initrd_vnode_read(vnode_t *v, uio_t *uio) {
  cpio_node_t *cn = (cpio_node_t *)v->v_data;
  int count = uio->uio_resid;
  int error = uiomove(cn->c_data, cn->c_size, uio);

  if (error < 0)
    return -error;

  return count - uio->uio_resid;
}

static int initrd_mount(mount_t *m) {
  vnode_t *root = vnode_new(V_DIR, &initrd_ops);
  root->v_data = (void *)root_node;
  root->v_mount = m;
  m->mnt_data = root;
  return 0;
}

static int initrd_root(mount_t *m, vnode_t **v) {
  *v = m->mnt_data;
  vnode_ref(*v);
  return 0;
}

static int initrd_init(vfsconf_t *vfc) {
  vfs_domount(vfc, vfs_root_initrd_vnode);
  return 0;
}

intptr_t ramdisk_get_start() {
  char *s = kenv_get("rd_start");
  if (s == NULL)
    return 0;
  int s_len = strlen(s);
  return strtoul(s + s_len - 8, NULL, 16);
}

unsigned ramdisk_get_size() {
  char *s = kenv_get("rd_size");
  return s ? strtoul(s, NULL, 0) : 0;
}

void ramdisk_init() {
  unsigned rd_size = ramdisk_get_size();

  TAILQ_INIT(&initrd_head);
  kmalloc_init(mp);
  kmalloc_add_pages(mp, 2);

  if (rd_size) {
    initrd_ops.v_lookup = initrd_vnode_lookup;
    initrd_ops.v_read = initrd_vnode_read;
    log("parsing cpio archive of %zu bytes", rd_size);
    read_cpio_archive();
    initrd_build_tree_and_names();
  }
}

void ramdisk_dump() {
  cpio_node_t *it;

  TAILQ_FOREACH (it, &initrd_head, c_list) { cpio_node_dump(it); }
}

static vfsops_t initrd_vfsops = {
  .vfs_mount = initrd_mount, .vfs_root = initrd_root, .vfs_init = initrd_init};

static vfsconf_t initrd_conf = {.vfc_name = "initrd",
                                .vfc_vfsops = &initrd_vfsops};

SET_ENTRY(vfsconf, initrd_conf);
