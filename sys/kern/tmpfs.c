#define KL_LOG KL_FILESYS
#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/kmem.h>

#define TMPFS_NAME_MAX 64

#define BLOCK_SIZE PAGESIZE
#define BLOCK_MASK (BLOCK_SIZE - 1)
#define ROUND_BLOCK(x)                                                         \
  (((x) + BLOCK_MASK) & ~BLOCK_MASK) /* Round up to block. */

#define INDIR_LEVELS 2
#define INDIR_IN_BLK_LOG 10 /* Logarithm of number of indirects in a block. */

#define DIRECT_BLK_NO 6              /* Number of direct block addresses. */
#define INDIRECT_BLK_NO INDIR_LEVELS /* Number of indirect block addresses. */

typedef struct tmpfs_dirent {
  TAILQ_ENTRY(tmpfs_dirent) tfd_entries; /* node on dirent list */
  struct tmpfs_node *tfd_node;           /* pointer to the file's node */
  size_t tfd_namelen;            /* number of bytes occupied in array below */
  char tfd_name[TMPFS_NAME_MAX]; /* name of file */
} tmpfs_dirent_t;

typedef TAILQ_HEAD(, tmpfs_dirent) tmpfs_dirent_list_t;

typedef struct tmpfs_node {
  vnode_t *tfn_vnode;   /* corresponding v-node */
  vnodetype_t tfn_type; /* node type */

  /* Node attributes (as in vattr) */
  mode_t tfn_mode;   /* node protection mode */
  nlink_t tfn_links; /* number of file hard links */
  ino_t tfn_ino;     /* node identifier */
  size_t tfn_size;   /* file size in bytes */

  /* Data that is only applicable to a particular type. */
  union {
    struct {
      struct tmpfs_node *parent;   /* Parent directory. */
      tmpfs_dirent_list_t dirents; /* List of directory entries. */
    } tfn_dir;
    struct {
      void *direct_blocks[DIRECT_BLK_NO];
      void *indirect_blocks[INDIRECT_BLK_NO];
      size_t nblocks; /* Number of blocks used by this file. */
    } tfn_reg;
  };
} tmpfs_node_t;

typedef struct tmpfs_mount {
  tmpfs_node_t *tfm_root;
  mtx_t tfm_lock;
  ino_t tfm_next_ino;
} tmpfs_mount_t;

static void *alloc_block(tmpfs_mount_t *tfm) {
  return kmem_alloc(PAGESIZE, M_ZERO);
}

static void free_block(tmpfs_mount_t *tfm, void *ptr) {
  kmem_free(ptr, PAGESIZE);
}

static POOL_DEFINE(P_TMPFS_NODE, "tmpfs node", sizeof(tmpfs_node_t));
static POOL_DEFINE(P_TMPFS_DIRENT, "tmpfs dirent", sizeof(tmpfs_dirent_t));

/* XXX: Temporary solution. There should be dedicated allocator for mount
 * points. */
static tmpfs_mount_t tmpfs;

/* Functions to convert VFS structures to tmpfs internal ones. */
static inline tmpfs_mount_t *TMPFS_ROOT_OF(mount_t *mp) {
  return (tmpfs_mount_t *)mp->mnt_data;
}

static inline tmpfs_node_t *TMPFS_NODE_OF(vnode_t *vp) {
  return (tmpfs_node_t *)vp->v_data;
}

/* Prototypes for internal routines. */
static void tmpfs_attach_vnode(tmpfs_node_t *tfn, mount_t *mp);
static tmpfs_node_t *tmpfs_new_node(tmpfs_mount_t *tfm, vnodetype_t ntype);
static void tmpfs_free_node(tmpfs_mount_t *tfm, tmpfs_node_t *tfn);
static int tmpfs_create_file(vnode_t *dv, vnode_t **vp, vnodetype_t ntype,
                             const char *name);
static int tmpfs_get_vnode(mount_t *mp, tmpfs_node_t *tfn, vnode_t **vp);
static int tmpfs_alloc_dirent(const char *name, tmpfs_dirent_t **dep);
static tmpfs_dirent_t *tmpfs_dir_lookup(tmpfs_node_t *tfn,
                                        const componentname_t *cn);
static void tmpfs_dir_detach(tmpfs_node_t *dv, tmpfs_dirent_t *de);

static void *tmpfs_reg_get_blk(tmpfs_node_t *v, size_t bn);
static int tmpfs_reg_resize(tmpfs_mount_t *tfm, tmpfs_node_t *v,
                            size_t newsize);

/* tmpfs readdir operations */

static void *tmpfs_dirent_next(vnode_t *v, void *it) {
  assert(it != NULL);
  if (it == DIRENT_DOT)
    return DIRENT_DOTDOT;
  if (it == DIRENT_DOTDOT)
    return TAILQ_FIRST(&TMPFS_NODE_OF(v)->tfn_dir.dirents);
  return TAILQ_NEXT((tmpfs_dirent_t *)it, tfd_entries);
}

static size_t tmpfs_dirent_namlen(vnode_t *v, void *it) {
  assert(it != NULL);
  if (it == DIRENT_DOT)
    return 1;
  if (it == DIRENT_DOTDOT)
    return 2;
  return ((tmpfs_dirent_t *)it)->tfd_namelen;
}

static void tmpfs_to_dirent(vnode_t *v, void *it, dirent_t *dir) {
  assert(it != NULL);
  tmpfs_node_t *node;
  const char *name;
  if (it == DIRENT_DOT) {
    node = TMPFS_NODE_OF(v);
    name = ".";
  } else if (it == DIRENT_DOTDOT) {
    node = TMPFS_NODE_OF(v)->tfn_dir.parent;
    name = "..";
  } else {
    node = ((tmpfs_dirent_t *)it)->tfd_node;
    name = ((tmpfs_dirent_t *)it)->tfd_name;
  }
  dir->d_fileno = node->tfn_ino;
  dir->d_type = vnode_to_dt(node->tfn_vnode);
  memcpy(dir->d_name, name, dir->d_namlen + 1);
}

static readdir_ops_t tmpfs_readdir_ops = {
  .next = tmpfs_dirent_next,
  .namlen_of = tmpfs_dirent_namlen,
  .convert = tmpfs_to_dirent,
};

/* tmpfs vnode operations */

static int tmpfs_vop_lookup(vnode_t *dv, componentname_t *cn, vnode_t **vp) {
  mount_t *mp = dv->v_mount;
  tmpfs_node_t *dnode = TMPFS_NODE_OF(dv);

  tmpfs_dirent_t *de = tmpfs_dir_lookup(dnode, cn);
  if (de == NULL)
    return ENOENT;

  return tmpfs_get_vnode(mp, de->tfd_node, vp);
}

static int tmpfs_vop_readdir(vnode_t *dv, uio_t *uio, void *state) {
  return readdir_generic(dv, uio, &tmpfs_readdir_ops);
}

static int tmpfs_vop_close(vnode_t *v, file_t *fp) {
  return 0;
}

static int tmpfs_vop_read(vnode_t *v, uio_t *uio) {
  tmpfs_node_t *node = TMPFS_NODE_OF(v);
  int error = 0;

  while (uio->uio_resid > 0) {
    size_t remaining = MIN(node->tfn_size - uio->uio_offset, uio->uio_resid);
    if (remaining == 0)
      break;

    size_t blkoffset = uio->uio_offset % BLOCK_SIZE;
    size_t len = MIN(BLOCK_SIZE - blkoffset, remaining);

    size_t bn = uio->uio_offset / BLOCK_SIZE;
    void *blk = tmpfs_reg_get_blk(node, bn);

    if ((error = uiomove((char *)blk + blkoffset, len, uio)))
      break;
  }
  return error;
}

static int tmpfs_vop_write(vnode_t *v, uio_t *uio) {
  tmpfs_mount_t *tfm = TMPFS_ROOT_OF(v->v_mount);
  tmpfs_node_t *node = TMPFS_NODE_OF(v);
  int error = 0;

  if (uio->uio_offset + uio->uio_resid > node->tfn_size) {
    if ((error = tmpfs_reg_resize(tfm, node, uio->uio_offset + uio->uio_resid)))
      return error;
  }

  while (uio->uio_resid > 0) {
    size_t blkoffset = uio->uio_offset % BLOCK_SIZE;
    size_t len = MIN(BLOCK_SIZE - blkoffset, uio->uio_resid);

    size_t bn = uio->uio_offset / BLOCK_SIZE;
    void *blk = tmpfs_reg_get_blk(node, bn);

    if ((error = uiomove((char *)blk + blkoffset, len, uio)))
      break;
  }
  return error;
}

static int tmpfs_vop_getattr(vnode_t *v, vattr_t *va) {
  tmpfs_node_t *node = TMPFS_NODE_OF(v);

  memset(va, 0, sizeof(vattr_t));
  va->va_mode = node->tfn_mode;
  va->va_nlink = node->tfn_links;
  va->va_ino = node->tfn_ino;
  va->va_size = node->tfn_size;
  return 0;
}

static int tmpfs_vop_setattr(vnode_t *v, vattr_t *va) {
  tmpfs_mount_t *tfm = TMPFS_ROOT_OF(v->v_mount);
  tmpfs_node_t *node = TMPFS_NODE_OF(v);

  if (va->va_size != (size_t)VNOVAL) {
    tmpfs_reg_resize(tfm, node, va->va_size);
  }

  return 0;
}

static int tmpfs_vop_create(vnode_t *dv, const char *name, vattr_t *va,
                            vnode_t **vp) {
  assert(S_ISREG(va->va_mode));
  return tmpfs_create_file(dv, vp, V_REG, name);
}

static int tmpfs_vop_remove(vnode_t *dv, vnode_t *v, const char *name) {
  tmpfs_node_t *dnode = TMPFS_NODE_OF(dv);
  tmpfs_dirent_t *de = tmpfs_dir_lookup(dnode, &COMPONENTNAME(name));
  assert(de != NULL);

  tmpfs_dir_detach(dnode, de);

  return 0;
}

static int tmpfs_vop_mkdir(vnode_t *dv, const char *name, vattr_t *va,
                           vnode_t **vp) {
  assert(S_ISDIR(va->va_mode));
  return tmpfs_create_file(dv, vp, V_DIR, name);
}

static int tmpfs_vop_rmdir(vnode_t *dv, vnode_t *v, const char *name) {
  tmpfs_node_t *dnode = TMPFS_NODE_OF(dv);
  tmpfs_dirent_t *de = tmpfs_dir_lookup(dnode, &COMPONENTNAME(name));
  assert(de != NULL);

  tmpfs_node_t *node = de->tfd_node;
  int error = 0;

  if (TAILQ_EMPTY(&node->tfn_dir.dirents)) {
    /* Decrement link count for the '.' entry. */
    node->tfn_links--;

    tmpfs_dir_detach(dnode, de);
  } else {
    error = ENOTEMPTY;
  }

  return error;
}

static int tmpfs_vop_reclaim(vnode_t *v) {
  tmpfs_mount_t *tfm = TMPFS_ROOT_OF(v->v_mount);
  tmpfs_node_t *node = TMPFS_NODE_OF(v);

  v->v_data = NULL;
  node->tfn_vnode = NULL;

  if (node->tfn_links == 0)
    tmpfs_free_node(tfm, node);

  return 0;
}

static vnodeops_t tmpfs_vnodeops = {.v_lookup = tmpfs_vop_lookup,
                                    .v_readdir = tmpfs_vop_readdir,
                                    .v_open = vnode_open_generic,
                                    .v_close = tmpfs_vop_close,
                                    .v_read = tmpfs_vop_read,
                                    .v_write = tmpfs_vop_write,
                                    .v_seek = vnode_seek_generic,
                                    .v_getattr = tmpfs_vop_getattr,
                                    .v_setattr = tmpfs_vop_setattr,
                                    .v_create = tmpfs_vop_create,
                                    .v_remove = tmpfs_vop_remove,
                                    .v_mkdir = tmpfs_vop_mkdir,
                                    .v_rmdir = tmpfs_vop_rmdir,
                                    .v_access = vnode_access_generic,
                                    .v_reclaim = tmpfs_vop_reclaim};

/* tmpfs internal routines */

/*
 * tmpfs_attach_vnode: init v-node and associate with existing inode.
 */
static void tmpfs_attach_vnode(tmpfs_node_t *tfn, mount_t *mp) {
  vnode_t *vn = vnode_new(tfn->tfn_type, &tmpfs_vnodeops, tfn);
  vn->v_mount = mp;
  vn->v_data = tfn;
  vn->v_type = tfn->tfn_type;
  vn->v_ops = &tmpfs_vnodeops;

  tfn->tfn_vnode = vn;
}

/*
 * tmpfs_new_node: create new inode of a specified type and attach the vnode.
 */
static tmpfs_node_t *tmpfs_new_node(tmpfs_mount_t *tfm, vnodetype_t ntype) {
  tmpfs_node_t *node = pool_alloc(P_TMPFS_NODE, M_ZERO);
  node->tfn_vnode = NULL;
  node->tfn_type = ntype;
  node->tfn_links = 0;
  node->tfn_size = 0;

  mtx_lock(&tfm->tfm_lock);
  node->tfn_ino = tfm->tfm_next_ino++;
  mtx_unlock(&tfm->tfm_lock);

  switch (node->tfn_type) {
    case V_DIR:
      TAILQ_INIT(&node->tfn_dir.dirents);

      /* Extra link count for the '.' entry. */
      node->tfn_links++;
      break;
    case V_REG:
      node->tfn_reg.nblocks = 0;
      break;
    default:
      panic("bad node type %d", node->tfn_type);
  }

  return node;
}

/*
 * tmpfs_free_node: remove the inode from a list in the mount point and
 * destroy the inode structures.
 */
static void tmpfs_free_node(tmpfs_mount_t *tfm, tmpfs_node_t *tfn) {
  switch (tfn->tfn_type) {
    case V_REG:
      tmpfs_reg_resize(tfm, tfn, 0);
      break;
    default:
      break;
  }
  pool_free(P_TMPFS_NODE, tfn);
}

/*
 * tmpfs_create_file: create a new file of specified type and adds it
 * into the parent directory.
 */
static int tmpfs_create_file(vnode_t *dv, vnode_t **vp, vnodetype_t ntype,
                             const char *name) {
  tmpfs_node_t *dnode = TMPFS_NODE_OF(dv);
  tmpfs_dirent_t *de;
  int error = 0;

  /* Allocate a new directory entry for the new file. */
  error = tmpfs_alloc_dirent(name, &de);
  if (error)
    return error;

  tmpfs_node_t *node = tmpfs_new_node(TMPFS_ROOT_OF(dv->v_mount), ntype);
  tmpfs_attach_vnode(node, dv->v_mount);

  /* Attach directory entry */
  node->tfn_links++;
  de->tfd_node = node;
  TAILQ_INSERT_TAIL(&dnode->tfn_dir.dirents, de, tfd_entries);
  dnode->tfn_size += sizeof(tmpfs_dirent_t);

  /* If directory set parent and increase the link count of parent. */
  if (node->tfn_type == V_DIR) {
    node->tfn_dir.parent = dnode;
    dnode->tfn_links++;
  }

  *vp = node->tfn_vnode;
  return error;
}

/*
 * tmpfs_get_vnode: get a v-node with usecnt incremented.
 */
static int tmpfs_get_vnode(mount_t *mp, tmpfs_node_t *tfn, vnode_t **vp) {
  vnode_t *vn = tfn->tfn_vnode;
  if (vn == NULL) {
    tmpfs_attach_vnode(tfn, mp);
  } else {
    vnode_hold(vn);
  }
  *vp = tfn->tfn_vnode;
  return 0;
}

/*
 * tmpfs_alloc_dirent: allocate a new directory entry.
 */
static int tmpfs_alloc_dirent(const char *name, tmpfs_dirent_t **dep) {
  size_t namelen = strlen(name);
  if (namelen + 1 > TMPFS_NAME_MAX)
    return ENAMETOOLONG;

  tmpfs_dirent_t *dirent = pool_alloc(P_TMPFS_DIRENT, M_ZERO);
  dirent->tfd_node = NULL;
  dirent->tfd_namelen = namelen;
  memcpy(dirent->tfd_name, name, namelen + 1);

  *dep = dirent;
  return 0;
}

static tmpfs_dirent_t *tmpfs_dir_lookup(tmpfs_node_t *tfn,
                                        const componentname_t *cn) {
  tmpfs_dirent_t *de;
  TAILQ_FOREACH (de, &tfn->tfn_dir.dirents, tfd_entries) {
    if (componentname_equal(cn, de->tfd_name))
      return de;
  }
  return NULL;
}

/*
 * tmpfs_dir_detach: disassociate directory entry and its node and and detach
 * the entry from the directory.
 */
static void tmpfs_dir_detach(tmpfs_node_t *dv, tmpfs_dirent_t *de) {
  tmpfs_node_t *v = de->tfd_node;
  assert(v->tfn_links > 0);
  v->tfn_links--;

  /* If directory - decrease the link count of parent. */
  if (v->tfn_type == V_DIR) {
    v->tfn_dir.parent = NULL;
    dv->tfn_links--;
  }
  de->tfd_node = NULL;
  TAILQ_REMOVE(&dv->tfn_dir.dirents, de, tfd_entries);
  dv->tfn_size -= sizeof(tmpfs_dirent_t);
  pool_free(P_TMPFS_DIRENT, de);
}

/*
 * tmpfs_dir_detach: get sequence of block pointers and offsets pointing to the
 * data block with number bn.
 */
static int tmpfs_reg_get_blk_indir(tmpfs_node_t *v, size_t bn, void **blkptr,
                                   size_t *blkoff) {
  if (bn < DIRECT_BLK_NO) {
    blkptr[0] = v->tfn_reg.direct_blocks;
    blkoff[0] = bn;
    return 1;
  }

  int levels = 0;
  bn -= DIRECT_BLK_NO;

  int log_blockcnt = 0;
  int i = INDIR_LEVELS;
  while (i > 0) {
    log_blockcnt += INDIR_IN_BLK_LOG;
    size_t blockcnt = 1 << log_blockcnt;
    if (bn < blockcnt)
      break;
    bn -= blockcnt;
    i--;
  }

  /* File too big */
  assert(i != 0);

  blkoff[levels] = INDIR_LEVELS - i;
  levels++;

  for (; i <= INDIR_LEVELS; i++) {
    log_blockcnt -= INDIR_IN_BLK_LOG;
    blkoff[levels] = (bn >> log_blockcnt) & BLOCK_MASK;
    levels++;
  }

  void **bp = v->tfn_reg.indirect_blocks;
  for (i = 0; i < levels; i++) {
    blkptr[i] = bp;

    /* If block doesn't exists, return NULL */
    if (bp)
      bp = bp[blkoff[i]];
  }

  return levels;
}

static int tmpfs_reg_alloc_blk(tmpfs_mount_t *tfm, tmpfs_node_t *v, size_t bn,
                               size_t *alloc_no) {
  void *ptrs[INDIR_LEVELS + 1];
  size_t indirs[INDIR_LEVELS + 1];
  int levels = tmpfs_reg_get_blk_indir(v, bn, ptrs, indirs);

  for (int i = 0; i < levels; i++) {
    void *bp = ((void **)ptrs[i])[indirs[i]];

    /* Last block should not be allocated */
    if (i == levels - 1)
      assert(bp == 0);

    if (bp == 0) {
      bp = alloc_block(tfm);
      if (!bp)
        return ENOMEM;
      (*alloc_no)++;
      ((void **)ptrs[i])[indirs[i]] = bp;
      if (i != levels - 1)
        ptrs[i + 1] = bp;
    }
  }
  return 0;
}

static void tmpfs_reg_free_blk(tmpfs_mount_t *tfm, tmpfs_node_t *v, size_t bn,
                               size_t *disalloc_no) {
  void *ptrs[INDIR_LEVELS + 1];
  size_t indirs[INDIR_LEVELS + 1];
  int levels = tmpfs_reg_get_blk_indir(v, bn, ptrs, indirs);

  free_block(tfm, ((void **)ptrs[levels - 1])[indirs[levels - 1]]);
  (*disalloc_no)++;
  bool last_freed = true;
  for (int i = levels - 1; i >= 0; i--) {
    if (last_freed)
      ((void **)ptrs[i])[indirs[i]] = NULL;
    if (i > 0 && last_freed && indirs[i] == 0) {
      free_block(tfm, ptrs[i]);
      (*disalloc_no)++;
    } else {
      last_freed = false;
    }
  }
}

static void *tmpfs_reg_get_blk(tmpfs_node_t *v, size_t bn) {
  void *ptrs[INDIR_LEVELS + 1];
  size_t indirs[INDIR_LEVELS + 1];
  int levels = tmpfs_reg_get_blk_indir(v, bn, ptrs, indirs);

  return ((void **)ptrs[levels - 1])[indirs[levels - 1]];
}

/*
 * tmpfs_dir_detach: resize regular file and possibly allocate new blocks.
 */
static int tmpfs_reg_resize(tmpfs_mount_t *tfm, tmpfs_node_t *v,
                            size_t newsize) {
  assert(v->tfn_type == V_REG);

  int error;
  size_t oldsize = v->tfn_size;
  size_t old_data_blocks = ROUND_BLOCK(oldsize) / BLOCK_SIZE;
  size_t new_data_blocks = ROUND_BLOCK(newsize) / BLOCK_SIZE;

  if (new_data_blocks > old_data_blocks) {
    size_t bn = old_data_blocks;
    size_t alloc_no = 0;
    while (bn < new_data_blocks) {
      if ((error = tmpfs_reg_alloc_blk(tfm, v, bn, &alloc_no)))
        return error;
      bn++;
    }
    v->tfn_reg.nblocks += alloc_no;
  } else if (newsize < oldsize) {
    size_t bn = old_data_blocks - 1;
    size_t disalloc_no = 0;
    while (bn + 1 > new_data_blocks) {
      tmpfs_reg_free_blk(tfm, v, bn, &disalloc_no);
      bn--;
    }
    v->tfn_reg.nblocks -= disalloc_no;
    /* If the file is not being truncated to a block boundry, the contents of
     * the partial block following the end of the file must be zero'ed */
    size_t blkoffset = newsize % BLOCK_SIZE;
    if (blkoffset != 0) {
      void *bp = tmpfs_reg_get_blk(v, bn);
      memset(bp + blkoffset, 0, BLOCK_SIZE - blkoffset);
    }
  }

  v->tfn_size = newsize;
  return 0;
}

/* tmpfs vfs operations */

static int tmpfs_mount(mount_t *mp) {
  /* Allocate the tmpfs mount structure and fill it. */
  tmpfs_mount_t *tfm = &tmpfs;

  tfm->tfm_lock = MTX_INITIALIZER(LK_RECURSE);
  tfm->tfm_next_ino = 2;
  mp->mnt_data = tfm;

  /* Allocate the root node. */
  tmpfs_node_t *root = tmpfs_new_node(tfm, V_DIR);
  tmpfs_attach_vnode(root, mp);
  root->tfn_dir.parent = root; /* Parent of the root node is itself. */
  root->tfn_links++; /* Extra link, because root has no directory entry. */

  tfm->tfm_root = root;
  vnode_drop(root->tfn_vnode);
  return 0;
}

static int tmpfs_root(mount_t *mp, vnode_t **vp) {
  tmpfs_mount_t *tfm = TMPFS_ROOT_OF(mp);
  return tmpfs_get_vnode(mp, tfm->tfm_root, vp);
}

static int tmpfs_init(vfsconf_t *vfc) {
  vnodeops_init(&tmpfs_vnodeops);
  return 0;
}

static vfsops_t tmpfs_vfsops = {
  .vfs_mount = tmpfs_mount, .vfs_root = tmpfs_root, .vfs_init = tmpfs_init};

static vfsconf_t tmpfs_conf = {.vfc_name = "tmpfs",
                               .vfc_vfsops = &tmpfs_vfsops};

SET_ENTRY(vfsconf, tmpfs_conf);
