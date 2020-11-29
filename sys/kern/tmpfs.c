#define KL_LOG KL_FILESYS
#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/cred.h>

#define TMPFS_NAME_MAX 64

#define BLOCK_SIZE PAGESIZE
#define BLOCK_MASK (BLOCK_SIZE - 1)
#define BLKNO(x) ((x) / BLOCK_SIZE)
#define BLKOFF(x) ((x) % BLOCK_SIZE)
#define NBLOCKS(x) (((x) + BLOCK_SIZE - 1) / BLOCK_SIZE)

#define PTR_IN_BLK (BLOCK_SIZE / sizeof(blkptr_t))

#define DIRECT_BLK_NO 6   /* Number of direct block addresses. */
#define INDIRECT_BLK_NO 2 /* Number of indirect block addresses. */

#define L1_BLK_NO PTR_IN_BLK
#define L2_BLK_NO (PTR_IN_BLK * PTR_IN_BLK)

typedef struct _blk *blkptr_t;

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
  uid_t tfn_uid;     /* owner of file */
  gid_t tfn_gid;     /* group of file */

  /* Data that is only applicable to a particular type. */
  union {
    struct {
      struct tmpfs_node *parent;   /* Parent directory. */
      tmpfs_dirent_list_t dirents; /* List of directory entries. */
    } tfn_dir;
    struct {
      blkptr_t direct[DIRECT_BLK_NO];
      blkptr_t *l1indirect;
      blkptr_t **l2indirect;
      size_t nblocks; /* Number of blocks used by this file. */
    } tfn_reg;
    struct {
      char *link;
    } tfn_lnk;
  };
} tmpfs_node_t;

typedef struct tmpfs_mount {
  tmpfs_node_t *tfm_root;
  mtx_t tfm_lock;
  ino_t tfm_next_ino;
} tmpfs_mount_t;

static int alloc_block(tmpfs_mount_t *tfm, tmpfs_node_t *v, blkptr_t *blkptrp) {
  assert(blkptrp != NULL);

  if (!(*blkptrp)) {
    blkptr_t blkptr = kmem_alloc(PAGESIZE, M_ZERO);
    if (!blkptr)
      return ENOMEM;
    v->tfn_reg.nblocks++;
    *blkptrp = blkptr;
  }

  return 0;
}

static void free_block(tmpfs_mount_t *tfm, tmpfs_node_t *v, blkptr_t *blkptrp) {
  assert(blkptrp != NULL);

  if (*blkptrp) {
    kmem_free(*blkptrp, PAGESIZE);
    *blkptrp = NULL;
    v->tfn_reg.nblocks--;
  }
}

static KMALLOC_DEFINE(M_TMPFS, "tmpfs");

/* This simplified string allocator will do for now. */
static char *alloc_str(tmpfs_mount_t *tfm, size_t len) {
  return kmalloc(M_TMPFS, len, 0);
}

static void free_str(tmpfs_mount_t *tfm, char *str) {
  kfree(M_TMPFS, str);
}

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
static tmpfs_node_t *tmpfs_new_node(tmpfs_mount_t *tfm, vattr_t *va,
                                    vnodetype_t ntype);
static void tmpfs_free_node(tmpfs_mount_t *tfm, tmpfs_node_t *tfn);
static int tmpfs_create_file(vnode_t *dv, vnode_t **vp, vattr_t *va,
                             vnodetype_t ntype, componentname_t *cn);
static void tmpfs_dir_attach(tmpfs_node_t *dnode, tmpfs_dirent_t *de,
                             tmpfs_node_t *node);
static int tmpfs_get_vnode(mount_t *mp, tmpfs_node_t *tfn, vnode_t **vp);
static int tmpfs_alloc_dirent(const char *name, size_t namelen,
                              tmpfs_dirent_t **dep);
static tmpfs_dirent_t *tmpfs_dir_lookup(tmpfs_node_t *tfn,
                                        const componentname_t *cn);
static void tmpfs_dir_detach(tmpfs_node_t *dv, tmpfs_dirent_t *de);

static blkptr_t *tmpfs_reg_get_blk(tmpfs_node_t *v, size_t blkno);
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
  dir->d_type = vt2dt(node->tfn_type);
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

  if (componentname_equal(cn, "..")) {
    tmpfs_node_t *pnode = dnode->tfn_dir.parent;
    return tmpfs_get_vnode(mp, pnode, vp);
  } else if (componentname_equal(cn, ".")) {
    vnode_hold(dv);
    *vp = dv;
    return 0;
  }

  tmpfs_dirent_t *de = tmpfs_dir_lookup(dnode, cn);
  if (de == NULL)
    return ENOENT;

  return tmpfs_get_vnode(mp, de->tfd_node, vp);
}

static int tmpfs_vop_readdir(vnode_t *dv, uio_t *uio) {
  return readdir_generic(dv, uio, &tmpfs_readdir_ops);
}

static int tmpfs_vop_close(vnode_t *v, file_t *fp) {
  return 0;
}

static int tmpfs_vop_read(vnode_t *v, uio_t *uio, int ioflag) {
  tmpfs_node_t *node = TMPFS_NODE_OF(v);
  size_t remaining;
  int error = 0;

  if (node->tfn_type == V_DIR)
    return EISDIR;
  if (node->tfn_type != V_REG)
    return EOPNOTSUPP;

  if (node->tfn_size <= (size_t)uio->uio_offset)
    return 0;

  while ((remaining = MIN(node->tfn_size - uio->uio_offset, uio->uio_resid))) {
    size_t blkoff = BLKOFF(uio->uio_offset);
    size_t len = MIN(BLOCK_SIZE - blkoff, remaining);
    size_t blkno = BLKNO(uio->uio_offset);
    void *blk = *tmpfs_reg_get_blk(node, blkno);
    if ((error = uiomove(blk + blkoff, len, uio)))
      break;
  }

  return error;
}

static int tmpfs_vop_write(vnode_t *v, uio_t *uio, int ioflag) {
  tmpfs_mount_t *tfm = TMPFS_ROOT_OF(v->v_mount);
  tmpfs_node_t *node = TMPFS_NODE_OF(v);
  int error = 0;

  if (node->tfn_type == V_DIR)
    return EISDIR;
  if (node->tfn_type != V_REG)
    return EOPNOTSUPP;

  if (ioflag & IO_APPEND)
    uio->uio_offset = node->tfn_size;

  if (uio->uio_offset + uio->uio_resid > node->tfn_size)
    if ((error = tmpfs_reg_resize(tfm, node, uio->uio_offset + uio->uio_resid)))
      return error;

  while (uio->uio_resid > 0) {
    size_t blkoff = BLKOFF(uio->uio_offset);
    size_t len = MIN(BLOCK_SIZE - blkoff, uio->uio_resid);
    size_t blkno = BLKNO(uio->uio_offset);
    void *blk = *tmpfs_reg_get_blk(node, blkno);
    if ((error = uiomove(blk + blkoff, len, uio)))
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
  va->va_uid = node->tfn_uid;
  va->va_gid = node->tfn_gid;
  va->va_size = node->tfn_size;
  return 0;
}

static int tmpfs_chmod(tmpfs_node_t *node, mode_t mode, cred_t *cred) {
  if (!cred_can_chmod(node->tfn_uid, node->tfn_gid, cred, mode))
    return EPERM;

  node->tfn_mode = (node->tfn_mode & ~ALLPERMS) | (mode & ALLPERMS);
  return 0;
}

static int tmpfs_chown(tmpfs_node_t *node, uid_t uid, gid_t gid, cred_t *cred) {
  if (!cred_can_chown(node->tfn_uid, cred, uid, gid))
    return EPERM;

  if (uid != (uid_t)-1) {
    node->tfn_uid = uid;
    node->tfn_mode &= ~S_ISUID; /* clear set-user-ID */
  }

  if (gid != (gid_t)-1) {
    node->tfn_gid = gid;
    node->tfn_mode &= ~S_ISGID; /* clear set-group-ID */
  }
  return 0;
}

static int tmpfs_vop_setattr(vnode_t *v, vattr_t *va, cred_t *cred) {
  tmpfs_mount_t *tfm = TMPFS_ROOT_OF(v->v_mount);
  tmpfs_node_t *node = TMPFS_NODE_OF(v);

  if (va->va_size != (size_t)VNOVAL)
    tmpfs_reg_resize(tfm, node, va->va_size);

  if (va->va_mode != (mode_t)VNOVAL) {
    int error;
    if ((error = tmpfs_chmod(node, va->va_mode, cred)))
      return error;
  }

  if (va->va_uid != (uid_t)VNOVAL || va->va_gid != (gid_t)VNOVAL)
    return tmpfs_chown(node, va->va_uid, va->va_gid, cred);

  return 0;
}

static int tmpfs_vop_create(vnode_t *dv, componentname_t *cn, vattr_t *va,
                            vnode_t **vp) {
  assert(S_ISREG(va->va_mode));
  return tmpfs_create_file(dv, vp, va, V_REG, cn);
}

static int tmpfs_vop_remove(vnode_t *dv, vnode_t *v, componentname_t *cn) {
  tmpfs_node_t *dnode = TMPFS_NODE_OF(dv);
  tmpfs_dirent_t *de = tmpfs_dir_lookup(dnode, cn);
  assert(de != NULL);

  tmpfs_dir_detach(dnode, de);

  return 0;
}

static int tmpfs_vop_mkdir(vnode_t *dv, componentname_t *cn, vattr_t *va,
                           vnode_t **vp) {
  assert(S_ISDIR(va->va_mode));
  return tmpfs_create_file(dv, vp, va, V_DIR, cn);
}

static int tmpfs_vop_rmdir(vnode_t *dv, vnode_t *v, componentname_t *cn) {
  tmpfs_node_t *dnode = TMPFS_NODE_OF(dv);
  tmpfs_dirent_t *de = tmpfs_dir_lookup(dnode, cn);
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

static int tmpfs_vop_readlink(vnode_t *v, uio_t *uio) {
  assert(v->v_type == V_LNK);
  tmpfs_node_t *node = TMPFS_NODE_OF(v);
  return uiomove_frombuf(node->tfn_lnk.link,
                         MIN((size_t)node->tfn_size, uio->uio_resid), uio);
}

static int tmpfs_vop_symlink(vnode_t *dv, componentname_t *cn, vattr_t *va,
                             char *target, vnode_t **vp) {
  tmpfs_mount_t *tfm = TMPFS_ROOT_OF(dv->v_mount);
  assert(S_ISLNK(va->va_mode));
  int error;
  size_t targetlen = strlen(target);
  char *str = NULL;

  assert(targetlen <= BLOCK_SIZE);
  if (targetlen > 0) {
    if (!(str = alloc_str(tfm, targetlen)))
      return ENOSPC;
    memcpy(str, target, targetlen);
  }

  if ((error = tmpfs_create_file(dv, vp, va, V_LNK, cn)))
    return error;

  tmpfs_node_t *node = TMPFS_NODE_OF(*vp);
  node->tfn_size = targetlen;
  node->tfn_lnk.link = str;
  return 0;
}

static int tmpfs_vop_link(vnode_t *dv, vnode_t *v, componentname_t *cn) {
  tmpfs_node_t *dnode = TMPFS_NODE_OF(dv);
  tmpfs_node_t *node = TMPFS_NODE_OF(v);
  tmpfs_dirent_t *de;
  int error;

  error = tmpfs_alloc_dirent(cn->cn_nameptr, cn->cn_namelen, &de);
  if (!error)
    tmpfs_dir_attach(dnode, de, node);
  return error;
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
                                    .v_reclaim = tmpfs_vop_reclaim,
                                    .v_readlink = tmpfs_vop_readlink,
                                    .v_symlink = tmpfs_vop_symlink,
                                    .v_link = tmpfs_vop_link};

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
 * tmpfs_new_node: create new inode of a specified type.
 */
static tmpfs_node_t *tmpfs_new_node(tmpfs_mount_t *tfm, vattr_t *va,
                                    vnodetype_t ntype) {
  tmpfs_node_t *node = kmalloc(M_TMPFS, sizeof(tmpfs_node_t), M_ZERO);
  node->tfn_vnode = NULL;
  node->tfn_mode = va->va_mode;
  node->tfn_type = ntype;
  node->tfn_links = 0;
  node->tfn_uid = va->va_uid;
  node->tfn_gid = va->va_gid;
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
    case V_LNK:
      node->tfn_lnk.link = NULL;
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
    case V_LNK:
      if (tfn->tfn_lnk.link)
        free_str(tfm, tfn->tfn_lnk.link);
      break;
    default:
      break;
  }

  kfree(M_TMPFS, tfn);
}

/*
 * tmpfs_create_file: create a new file of specified type and adds it
 * into the parent directory.
 */
static int tmpfs_create_file(vnode_t *dv, vnode_t **vp, vattr_t *va,
                             vnodetype_t ntype, componentname_t *cn) {
  tmpfs_node_t *dnode = TMPFS_NODE_OF(dv);
  tmpfs_dirent_t *de;
  int error = 0;

  /* Allocate a new directory entry for the new file. */
  error = tmpfs_alloc_dirent(cn->cn_nameptr, cn->cn_namelen, &de);
  if (error)
    return error;

  tmpfs_node_t *node = tmpfs_new_node(TMPFS_ROOT_OF(dv->v_mount), va, ntype);
  tmpfs_attach_vnode(node, dv->v_mount);

  /* Attach directory entry */
  tmpfs_dir_attach(dnode, de, node);

  *vp = node->tfn_vnode;
  return error;
}

/*
 * tmpfs_dir_attach: associate directory entry with a specified inode, and
 * attach the entry into the directory, specified by dnode.
 */
static void tmpfs_dir_attach(tmpfs_node_t *dnode, tmpfs_dirent_t *de,
                             tmpfs_node_t *node) {
  node->tfn_links++;
  de->tfd_node = node;
  TAILQ_INSERT_TAIL(&dnode->tfn_dir.dirents, de, tfd_entries);
  dnode->tfn_size += sizeof(tmpfs_dirent_t);

  /* If directory set parent and increase the link count of parent. */
  if (node->tfn_type == V_DIR) {
    node->tfn_dir.parent = dnode;
    dnode->tfn_links++;
  }
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
static int tmpfs_alloc_dirent(const char *name, size_t namelen,
                              tmpfs_dirent_t **dep) {
  if (namelen + 1 > TMPFS_NAME_MAX)
    return ENAMETOOLONG;

  tmpfs_dirent_t *dirent = kmalloc(M_TMPFS, sizeof(tmpfs_dirent_t), M_ZERO);
  dirent->tfd_node = NULL;
  dirent->tfd_namelen = namelen;
  memcpy(dirent->tfd_name, name, namelen);
  dirent->tfd_name[namelen] = 0;

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
  kfree(M_TMPFS, de);
}

/*
 * tmpfs_reg_expand_meta: expand indirect blocks metadata to store up to blkcnt
 * data blocks.
 */
static int tmpfs_reg_expand_meta(tmpfs_mount_t *tfm, tmpfs_node_t *v,
                                 size_t blkcnt) {
  int error;

  /* L1 */
  if (blkcnt <= DIRECT_BLK_NO)
    return 0;

  if ((error = alloc_block(tfm, v, (blkptr_t *)&v->tfn_reg.l1indirect)))
    return error;

  /* L2 */
  if (blkcnt <= DIRECT_BLK_NO + L1_BLK_NO)
    return 0;

  if ((error = alloc_block(tfm, v, (blkptr_t *)&v->tfn_reg.l2indirect)))
    return error;

  size_t l2blkcnt =
    (blkcnt - (DIRECT_BLK_NO + L1_BLK_NO) + PTR_IN_BLK - 1) / PTR_IN_BLK;
  for (size_t i = 0; i < l2blkcnt; i++) {
    if ((error = alloc_block(tfm, v, (blkptr_t *)&v->tfn_reg.l2indirect[i])))
      return error;
  }

  return 0;
}

/*
 * tmpfs_reg_shrink_meta: shrink indirect blocks metadata to store only blkcnt
 * data blocks.
 */
static void tmpfs_reg_shrink_meta(tmpfs_mount_t *tfm, tmpfs_node_t *v,
                                  size_t blkcnt) {
  /* L1 */
  if (blkcnt <= DIRECT_BLK_NO && v->tfn_reg.l1indirect)
    free_block(tfm, v, (blkptr_t *)&v->tfn_reg.l1indirect);

  /* L2 */
  if (!v->tfn_reg.l2indirect)
    return;

  /* Number of pointers in L2 block that will be still used. */
  size_t l2ptrcnt = 0;

  if (blkcnt > L1_BLK_NO + DIRECT_BLK_NO)
    l2ptrcnt =
      (blkcnt - (DIRECT_BLK_NO + L1_BLK_NO) + PTR_IN_BLK - 1) / PTR_IN_BLK;

  for (size_t i = l2ptrcnt; i < PTR_IN_BLK && v->tfn_reg.l2indirect[i]; i++)
    free_block(tfm, v, (blkptr_t *)&v->tfn_reg.l2indirect[i]);

  if (l2ptrcnt == 0)
    free_block(tfm, v, (blkptr_t *)&v->tfn_reg.l2indirect);
}

/*
 * tmpfs_reg_get_blk: returns pointer to pointer of data block with number
 * blkno.
 */
static blkptr_t *tmpfs_reg_get_blk(tmpfs_node_t *v, size_t blkno) {
  if (blkno < DIRECT_BLK_NO)
    return &v->tfn_reg.direct[blkno];

  blkno -= DIRECT_BLK_NO;
  if (blkno < L1_BLK_NO)
    return &v->tfn_reg.l1indirect[blkno];

  blkno -= L1_BLK_NO;
  blkptr_t *l1arr = v->tfn_reg.l2indirect[blkno / PTR_IN_BLK];
  return &l1arr[blkno % PTR_IN_BLK];
}

static int tmpfs_reg_alloc_blk(tmpfs_mount_t *tfm, tmpfs_node_t *v, size_t from,
                               size_t to) {
  int error;

  for (size_t blkno = from; blkno < to; blkno++) {
    blkptr_t *blk = tmpfs_reg_get_blk(v, blkno);
    assert(!(*blk));
    if ((error = alloc_block(tfm, v, blk)))
      return error;
  }

  return 0;
}

static void tmpfs_reg_free_blk(tmpfs_mount_t *tfm, tmpfs_node_t *v, size_t from,
                               size_t to) {
  for (size_t blkno = from; blkno < to; blkno++)
    free_block(tfm, v, tmpfs_reg_get_blk(v, blkno));
}

/*
 * tmpfs_reg_resize: resize regular file and possibly allocate new blocks.
 */
static int tmpfs_reg_resize(tmpfs_mount_t *tfm, tmpfs_node_t *v,
                            size_t newsize) {
  assert(v->tfn_type == V_REG);

  size_t oldsize = v->tfn_size;
  size_t oldblks = NBLOCKS(oldsize);
  size_t newblks = NBLOCKS(newsize);
  int error;

  if (newblks > oldblks) {
    if ((error = tmpfs_reg_expand_meta(tfm, v, newblks))) {
      tmpfs_reg_shrink_meta(tfm, v, oldblks);
      return error;
    }
    if ((error = tmpfs_reg_alloc_blk(tfm, v, oldblks, newblks))) {
      tmpfs_reg_free_blk(tfm, v, oldblks, newblks);
      tmpfs_reg_shrink_meta(tfm, v, oldblks);
      return error;
    }

  } else if (newsize < oldsize) {
    if (newblks < oldblks) {
      tmpfs_reg_free_blk(tfm, v, newblks, oldblks);
      tmpfs_reg_shrink_meta(tfm, v, newblks);
    }

    /* If the file is not being truncated to a block boundry, the contents of
     * the partial block following the end of the file must be zero'ed */
    size_t blkno = BLKNO(newsize);
    size_t blkoff = BLKOFF(newsize);
    if (blkoff) {
      blkptr_t *blk = tmpfs_reg_get_blk(v, blkno);
      memset((void *)*blk + blkoff, 0, BLOCK_SIZE - blkoff);
    }
  }

  v->tfn_size = newsize;
  return 0;
}

/* tmpfs vfs operations */

static int tmpfs_mount(mount_t *mp) {
  /* Allocate the tmpfs mount structure and fill it. */
  tmpfs_mount_t *tfm = &tmpfs;

  tfm->tfm_lock = MTX_INITIALIZER(LK_RECURSIVE);
  tfm->tfm_next_ino = 2;
  mp->mnt_data = tfm;

  /* Allocate the root node. */
  vattr_t va;
  vattr_null(&va);
  va.va_mode = S_IFDIR | (ACCESSPERMS & ~CMASK) | S_ISVTX;
  tmpfs_node_t *root = tmpfs_new_node(tfm, &va, V_DIR);
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
