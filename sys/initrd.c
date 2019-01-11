#define KL_LOG KL_FILESYS
#include <klog.h>
#include <errno.h>
#include <pool.h>
#include <stdc.h>
#include <cpio.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>
#include <file.h>
#include <vfs.h>
#include <linker_set.h>
#include <dirent.h>

typedef uint32_t cpio_dev_t;
typedef uint32_t cpio_ino_t;
typedef uint16_t cpio_mode_t;

typedef struct cpio_node cpio_node_t;
typedef TAILQ_HEAD(, cpio_node) cpio_list_t;

/* ramdisk related data that will be stored in v_data field of vnode */
struct cpio_node {
  TAILQ_ENTRY(cpio_node) c_list; /* link to global list of all ramdisk nodes */
  cpio_list_t c_children;        /* head of list of direct descendants */
  cpio_node_t *c_parent;         /* pointer to parent or NULL for root node */
  TAILQ_ENTRY(cpio_node) c_siblings; /* nodes that have the same parent */

  cpio_dev_t c_dev;
  cpio_ino_t c_ino;
  cpio_mode_t c_mode;
  nlink_t c_nlink;
  uid_t c_uid;
  gid_t c_gid;
  cpio_dev_t c_rdev;
  off_t c_size;
  time_t c_mtime;

  const char *c_path; /* contains exact path to file as archived by cpio */
  const char *c_name; /* contains name of file */
  void *c_data;
  /* Associated vnode. */
  vnode_t *c_vnode;
};

static POOL_DEFINE(P_INITRD, "initrd", sizeof(cpio_node_t));

static cpio_list_t initrd_head = TAILQ_HEAD_INITIALIZER(initrd_head);
static cpio_node_t *root_node;
static vnodeops_t initrd_vops;

extern int8_t __rd_start[];
extern int8_t __rd_end[];

extern char *kenv_get(const char *key);

static cpio_node_t *cpio_node_alloc(void) {
  cpio_node_t *node = pool_alloc(P_INITRD, PF_ZERO);
  TAILQ_INIT(&node->c_children);
  return node;
}

static void cpio_node_dump(cpio_node_t *cn) {
  klog("initrd entry '%s', inode: %lu:", cn->c_path, cn->c_ino);
  klog(" mode: %o, nlink: %d, uid: %d, gid: %d, size: %ld, mtime: %ld",
       cn->c_mode, cn->c_nlink, cn->c_uid, cn->c_gid, cn->c_size, cn->c_mtime);
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
    klog("wrong magic number: %o", c_magic);
    return false;
  }

  uint32_t c_ino = strntoul(hdr.c_ino, 8, NULL, 16);
  uint32_t c_mode = strntoul(hdr.c_mode, 8, NULL, 16);
  uint32_t c_uid = strntoul(hdr.c_uid, 8, NULL, 16);
  uint32_t c_gid = strntoul(hdr.c_gid, 8, NULL, 16);
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
  /* hardlink count: number of subdirectiories + 2 ('.', '..'), otherwise 1 */
  cpio->c_nlink = (CMTOFT(c_mode) == C_DIR) ? 2 : 1;
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

/* Extract last component of the path. */
static const char *basename(const char *path) {
  char *name = strrchr(path, '/');
  return name ? name + 1 : path;
}

static void read_cpio_archive(void) {
  void *tape = (void *)ramdisk_get_start();

  while (true) {
    cpio_node_t *node = cpio_node_alloc();
    if (!read_cpio_header(&tape, node) ||
        strcmp(node->c_path, CPIO_TRAILER) == 0) {
      pool_free(P_INITRD, node);
      break;
    }

    /* Take care of root node denoted with '.' in cpio archive. */
    if (strcmp(node->c_path, ".") == 0) {
      node->c_path = "";
      node->c_parent = node;
      root_node = node;
    }

    node->c_name = basename(node->c_path);

    TAILQ_INSERT_HEAD(&initrd_head, node, c_list);
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

  /* Check whether `p1` is valid filename (does not contain '/') */
  if (*p1 == '/')
    p1++; /* skip trailing '/' */
  return (strchr(p1, '/') == NULL);
}

static void initrd_build_tree(void) {
  cpio_node_t *parent, *child;

  TAILQ_FOREACH (parent, &initrd_head, c_list) {
    if (CMTOFT(parent->c_mode) != C_DIR)
      continue;
    TAILQ_FOREACH (child, &initrd_head, c_list) {
      if (parent == child)
        continue;
      if (is_direct_descendant(child->c_path, parent->c_path))
        TAILQ_INSERT_TAIL(&parent->c_children, child, c_siblings);
    }
  }
}

static ino_t initrd_enum_inodes(cpio_node_t *parent, ino_t ino) {
  cpio_node_t *it;
  parent->c_ino = ino++;
  TAILQ_FOREACH (it, &parent->c_children, c_siblings) {
    it->c_parent = parent;
    if (CMTOFT(it->c_mode) == C_DIR) {
      parent->c_nlink++;
      ino = initrd_enum_inodes(it, ino);
    } else {
      it->c_ino = ino++;
    }
  }
  return ino;
}

static int initrd_vnode_lookup(vnode_t *vdir, const char *name, vnode_t **res) {
  cpio_node_t *it;
  cpio_node_t *cn_dir = (cpio_node_t *)vdir->v_data;

  TAILQ_FOREACH (it, &cn_dir->c_children, c_siblings) {
    if (strcmp(name, it->c_name) == 0) {
      if (it->c_vnode) {
        *res = it->c_vnode;
      } else {
        /* Create new vnode */
        vnodetype_t type = V_REG;
        if (CMTOFT(it->c_mode) == C_DIR)
          type = V_DIR;
        *res = vnode_new(type, &initrd_vops, it);

        /* TODO: Only store a token (weak pointer) that allows looking up the
           vnode, otherwise the vnode will never get freed. */
        it->c_vnode = *res;
        vnode_hold(*res);
      }
      /* Reference for the caller */
      vnode_hold(*res);
      return 0;
    }
  }
  return -ENOENT;
}

static int initrd_vnode_read(vnode_t *v, uio_t *uio) {
  cpio_node_t *cn = (cpio_node_t *)v->v_data;
  int count = uio->uio_resid;
  int error = uiomove_frombuf(cn->c_data, cn->c_size, uio);

  if (error < 0)
    return -error;

  return count - uio->uio_resid;
}

static int initrd_vnode_getattr(vnode_t *v, vattr_t *va) {
  cpio_node_t *cn = (cpio_node_t *)v->v_data;
  va->va_mode = cn->c_mode;
  va->va_nlink = cn->c_nlink;
  va->va_uid = cn->c_uid;
  va->va_gid = cn->c_gid;
  va->va_size = cn->c_size;
  return 0;
}

static inline cpio_node_t *vn2cn(vnode_t *v) {
  return (cpio_node_t *)v->v_data;
}

static void *cpio_dirent_next(vnode_t *v, void *it) {
  assert(it != NULL);
  if (it == DIRENT_DOT)
    return DIRENT_DOTDOT;
  if (it == DIRENT_DOTDOT)
    return TAILQ_FIRST(&vn2cn(v)->c_children);
  return TAILQ_NEXT((cpio_node_t *)it, c_siblings);
}

static size_t cpio_dirent_namlen(vnode_t *v, void *it) {
  assert(it != NULL);
  if (it == DIRENT_DOT)
    return 1;
  if (it == DIRENT_DOTDOT)
    return 2;
  return strlen(((cpio_node_t *)it)->c_name);
}

static const unsigned ft2dt[16] = {[C_FIFO] = DT_FIFO, [C_CHR] = DT_CHR,
                                   [C_DIR] = DT_DIR,   [C_BLK] = DT_BLK,
                                   [C_REG] = DT_REG,   [C_CTG] = DT_UNKNOWN,
                                   [C_LNK] = DT_LNK,   [C_SOCK] = DT_SOCK};

static void cpio_to_dirent(vnode_t *v, void *it, dirent_t *dir) {
  assert(it != NULL);
  cpio_node_t *node;
  const char *name;
  if (it == DIRENT_DOT) {
    node = vn2cn(v);
    name = ".";
  } else if (it == DIRENT_DOTDOT) {
    node = vn2cn(v)->c_parent;
    name = "..";
  } else {
    node = (cpio_node_t *)it;
    name = node->c_name;
  }
  dir->d_fileno = node->c_ino;
  dir->d_type = ft2dt[CMTOFT(node->c_mode)];
  memcpy(dir->d_name, name, dir->d_namlen + 1);
}

static readdir_ops_t cpio_readdir_ops = {
  .next = cpio_dirent_next,
  .namlen_of = cpio_dirent_namlen,
  .convert = cpio_to_dirent,
};

static int initrd_vnode_readdir(vnode_t *v, uio_t *uio, void *state) {
  return readdir_generic(v, uio, &cpio_readdir_ops);
}

static int initrd_root(mount_t *m, vnode_t **v) {
  *v = m->mnt_data;
  vnode_hold(*v);
  return 0;
}

static int initrd_mount(mount_t *m) {
  vnode_t *root = vnode_new(V_DIR, &initrd_vops, root_node);
  root->v_mount = m;
  m->mnt_data = root;
  return 0;
}

static vnodeops_t initrd_vops = {.v_lookup = initrd_vnode_lookup,
                                 .v_readdir = initrd_vnode_readdir,
                                 .v_open = vnode_open_generic,
                                 .v_read = initrd_vnode_read,
                                 .v_seek = vnode_seek_generic,
                                 .v_getattr = initrd_vnode_getattr,
                                 .v_access = vnode_access_generic};

static int initrd_init(vfsconf_t *vfc) {
  /* Ramdisk start & end addresses are expected to be page aligned. */
  assert(is_aligned(ramdisk_get_start(), PAGESIZE));
  /* If the size is page aligned, the end address is as well. */
  assert(is_aligned(ramdisk_get_size(), PAGESIZE));

  vnodeops_init(&initrd_vops);

  klog("parsing cpio archive of %u bytes", ramdisk_get_size());
  read_cpio_archive();
  initrd_build_tree();
  initrd_enum_inodes(root_node, 2);
  return 0;
}

intptr_t ramdisk_get_start(void) {
  return (intptr_t)__rd_start;
}

unsigned ramdisk_get_size(void) {
  return (unsigned)__rd_end - (unsigned)__rd_start;
}

void ramdisk_dump(void) {
  cpio_node_t *it;

  TAILQ_FOREACH (it, &initrd_head, c_list) { cpio_node_dump(it); }
}

static vfsops_t initrd_vfsops = {
  .vfs_mount = initrd_mount, .vfs_root = initrd_root, .vfs_init = initrd_init};

static vfsconf_t initrd_conf = {.vfc_name = "initrd",
                                .vfc_vfsops = &initrd_vfsops};

SET_ENTRY(vfsconf, initrd_conf);
