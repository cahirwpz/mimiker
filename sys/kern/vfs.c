#define KL_LOG KL_VFS
#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/linker_set.h>
#include <sys/sysinit.h>
#include <sys/mimiker.h>
#include <sys/proc.h>
#include <sys/stat.h>

/* Internal state for a vnr operation. */
typedef struct {
  /* Arguments to vnr. */
  vnrop_t vs_op; /* vnr operation type */

  /* Results returned from lookup. */
  vnode_t *vs_vp;  /* vnode of result */
  vnode_t *vs_dvp; /* vnode of parent directory */

  componentname_t vs_cn;
  const char *vs_nextcn;
} vnrstate_t;

/* TODO: We probably need some fancier allocation, since eventually we should
 * start recycling vnodes */
static POOL_DEFINE(P_MOUNT, "vfs mount points", sizeof(mount_t));

/* The list of all installed filesystem types */
vfsconf_list_t vfsconf_list = TAILQ_HEAD_INITIALIZER(vfsconf_list);
mtx_t vfsconf_list_mtx = MTX_INITIALIZER(0);

/* The list of all mounts mounted */
typedef TAILQ_HEAD(, mount) mount_list_t;
static mount_list_t mount_list = TAILQ_HEAD_INITIALIZER(mount_list);
static mtx_t mount_list_mtx = MTX_INITIALIZER(0);

/* Default vfs operations */
static vfs_root_t vfs_default_root;
static vfs_statfs_t vfs_default_statfs;
static vfs_vget_t vfs_default_vget;
static vfs_init_t vfs_default_init;

/* Global root vnodes */
vnode_t *vfs_root_vnode;

static vnodeops_t vfs_root_ops = {};

static int vfs_register(vfsconf_t *vfc);

static void vfs_init(void) {
  vnodeops_init(&vfs_root_ops);

  vfs_root_vnode = vnode_new(V_DIR, &vfs_root_ops, NULL);

  /* Initialize available filesystem types. */
  SET_DECLARE(vfsconf, vfsconf_t);
  vfsconf_t **ptr;
  SET_FOREACH (ptr, vfsconf)
    vfs_register(*ptr);
}

vfsconf_t *vfs_get_by_name(const char *name) {
  SCOPED_MTX_LOCK(&vfsconf_list_mtx);

  vfsconf_t *vfc;
  TAILQ_FOREACH (vfc, &vfsconf_list, vfc_list)
    if (!strcmp(name, vfc->vfc_name))
      return vfc;
  return NULL;
}

/* Register a file system type */
static int vfs_register(vfsconf_t *vfc) {
  /* Check if this file system type was already registered */
  if (vfs_get_by_name(vfc->vfc_name))
    return EEXIST;

  WITH_MTX_LOCK (&vfsconf_list_mtx)
    TAILQ_INSERT_TAIL(&vfsconf_list, vfc, vfc_list);

  vfc->vfc_mountcnt = 0;

  /* Ensure the filesystem provides obligatory operations */
  assert(vfc->vfc_vfsops != NULL);
  assert(vfc->vfc_vfsops->vfs_mount != NULL);

  /* Use defaults for other operations, if not provided. */
  if (vfc->vfc_vfsops->vfs_root == NULL)
    vfc->vfc_vfsops->vfs_root = vfs_default_root;
  if (vfc->vfc_vfsops->vfs_statfs == NULL)
    vfc->vfc_vfsops->vfs_statfs = vfs_default_statfs;
  if (vfc->vfc_vfsops->vfs_vget == NULL)
    vfc->vfc_vfsops->vfs_vget = vfs_default_vget;
  if (vfc->vfc_vfsops->vfs_init == NULL)
    vfc->vfc_vfsops->vfs_init = vfs_default_init;

  /* Call init function for this vfs... */
  vfc->vfc_vfsops->vfs_init(vfc);

  return 0;
}

static int vfs_default_root(mount_t *m, vnode_t **v) {
  return ENOTSUP;
}

static int vfs_default_statfs(mount_t *m, statfs_t *sb) {
  return ENOTSUP;
}

static int vfs_default_vget(mount_t *m, ino_t ino, vnode_t **v) {
  return ENOTSUP;
}

static int vfs_default_init(vfsconf_t *vfc) {
  return 0;
}

mount_t *vfs_mount_alloc(vnode_t *v, vfsconf_t *vfc) {
  mount_t *m = pool_alloc(P_MOUNT, M_ZERO);

  m->mnt_vfc = vfc;
  m->mnt_vfsops = vfc->vfc_vfsops;
  vfc->vfc_mountcnt++; /* TODO: vfc_mtx? */
  m->mnt_data = NULL;

  m->mnt_vnodecovered = v;

  m->mnt_refcnt = 0;
  mtx_init(&m->mnt_mtx, 0);

  return m;
}

int vfs_domount(vfsconf_t *vfc, vnode_t *v) {
  int error;

  /* Start by checking whether this vnode can be used for mounting */
  if (v->v_type != V_DIR)
    return ENOTDIR;
  if (is_mountpoint(v))
    return EBUSY;

  /* TODO: Mark the vnode is in-progress of mounting? See VI_MOUNT in FreeBSD */

  mount_t *m = vfs_mount_alloc(v, vfc);

  /* Mount the filesystem. */
  if ((error = VFS_MOUNT(m)))
    return error;

  v->v_mountedhere = m;

  WITH_MTX_LOCK (&mount_list_mtx)
    TAILQ_INSERT_TAIL(&mount_list, m, mnt_list);

  /* Note that we do not need to ask the new mount for the root vnode! That
     V_DIR vnode which is at the mount point stays in place. The root vnode is
     requested during path lookup process, once we encounter a directory that
     has v_mountedhere set. */

  /* TODO: 'mountcheckdirs', which checks each process in the system, and
     updates their dirs to reflect on the mounted file system... */

  return 0;
}

int vfs_maybe_ascend(vnode_t **vp)
{
  vnode_t* v_covered;
  vnode_t* v = *vp;
  while (is_mounted(v)) {
    v_covered = v->v_mount->mnt_vnodecovered;
    vnode_hold(v_covered);
    vnode_drop(v);
    v = v_covered;
  }

  *vp = v;
  return 0;
}

/* If `*vp` is a mountpoint, then descend into the root of mounted filesys. */
static int vfs_maybe_descend(vnode_t **vp) {
  vnode_t *v_mntpt;
  vnode_t *v = *vp;
  while (is_mountpoint(v)) {
    int error = VFS_ROOT(v->v_mountedhere, &v_mntpt);
    vnode_put(v);
    if (error)
      return error;
    v = v_mntpt;
    /* No need to ref this vnode, VFS_ROOT already did it for us. */
    vnode_lock(v);
    *vp = v;
  }
  return 0;
}

bool componentname_equal(const componentname_t *cn, const char *name) {
  if (strlen(name) != cn->cn_namelen)
    return false;
  return strncmp(name, cn->cn_nameptr, cn->cn_namelen) == 0;
}

/* Call VOP_LOOKUP for a single lookup. */
static int vnr_lookup_once(vnrstate_t *state, vnode_t *searchdir,
                           vnode_t **foundvn_p) {
  vnode_t *foundvn;
  componentname_t *cn = &state->vs_cn;

  int error = VOP_LOOKUP(searchdir, cn, &foundvn);
  if (error) {
    /*
     * The entry was not found in the directory. This is valid
     * if we are creating an entry and are working
     * on the last component of the path name.
     */
    if (error == ENOENT && state->vs_op == VNR_CREATE &&
        cn->cn_flags & VNR_ISLASTPC) {
      vnode_unlock(searchdir);
      foundvn = NULL;
      error = 0;
    }
  } else {
    /* No need to ref this vnode, VOP_LOOKUP already did it for us. */
    vnode_unlock(searchdir);
    vnode_lock(foundvn);
    if (componentname_equal(cn, ".."))
      vfs_maybe_ascend(&foundvn);
    else
      vfs_maybe_descend(&foundvn);
  }

  *foundvn_p = foundvn;
  return error;
}

static void vnr_parse_component(vnrstate_t *state) {
  componentname_t *cn = &state->vs_cn;
  const char *name = state->vs_nextcn;

  /* Look for end of string or component separator. */
  cn->cn_nameptr = name;
  while (*name != '\0' && *name != '/')
    name++;
  cn->cn_namelen = name - cn->cn_nameptr;

  /* Skip component separators. */
  while (*name == '/')
    name++;

  /* Last component? */
  if (*name == '\0')
    cn->cn_flags |= VNR_ISLASTPC;

  state->vs_nextcn = name;
}

#define ASSERT_UNLOCKED \
  if (parentdir != searchdir) \
  { \
    vnode_lock(parentdir); \
    vnode_unlock(parentdir); \
  }

static int vfs_nameresolve(vnrstate_t *state) {
  /* TODO: This is a simplified implementation, and it does not support many
     required features! These include: relative paths, symlinks, parent dirs */
  int error;
  vnode_t *searchdir, *parentdir = NULL;
  componentname_t *cn = &state->vs_cn;

  if (state->vs_nextcn[0] == '\0')
    return ENOENT;

  if (strlen(state->vs_nextcn) >= PATH_MAX)
    return ENAMETOOLONG;

  if (strncmp(state->vs_nextcn, "/", 1) == 0) {
    searchdir = vfs_root_vnode;
    /* Drop leading slashes. */
    while (state->vs_nextcn[0] == '/')
      state->vs_nextcn++;
  } else {
    searchdir = proc_self()->p_cwd;
  }

  /* Establish the starting directory for lookup, and lock it. */
  if (searchdir->v_type != V_DIR)
    return ENOTDIR;

  /* Path was just "/". */
  vnode_hold(searchdir);
  vnode_lock(searchdir);

  if ((error = vfs_maybe_descend(&searchdir)))
    goto end;

  parentdir = searchdir;
  vnode_hold(parentdir);

  ASSERT_UNLOCKED

  if (state->vs_nextcn[0] == '\0')
  {
    vnode_unlock(searchdir);
    ASSERT_UNLOCKED
    state->vs_dvp = parentdir;
    state->vs_vp = searchdir;
    error = 0;
    goto end;
  }

  for(;;) {
    assert(state->vs_nextcn[0] != '/');
    assert(state->vs_nextcn[0] != '\0');

    /* Prepare the next path name component. */
    vnr_parse_component(state);
    if (state->vs_cn.cn_namelen > NAME_MAX) {
      error = ENAMETOOLONG;
      vnode_put(searchdir);
      goto end;
    }

    ASSERT_UNLOCKED

    if (!componentname_equal(cn, "."))
    {
      ASSERT_UNLOCKED

      vnode_drop(parentdir);
      parentdir = searchdir;
      searchdir = NULL;

      error = vnr_lookup_once(state, parentdir, &searchdir);
      if (error)
      {
        vnode_put(parentdir);
        goto end;
      }

      ASSERT_UNLOCKED
      /* Success with no object returned means we're creating something. */
      if (searchdir == NULL)
        break;
    }

    ASSERT_UNLOCKED
    if (cn->cn_flags & VNR_ISLASTPC)
      break;
  }

  if (searchdir != NULL && state->vs_op != VNR_DELETE)
    vnode_unlock(searchdir);

  /* Release the parent directory if is not needed. */
  if (state->vs_op == VNR_LOOKUP && parentdir != NULL) {
    vnode_drop(parentdir);
    parentdir = NULL;
  }

  if (parentdir != NULL && parentdir != searchdir)
    vnode_lock(parentdir);

  state->vs_vp = searchdir;
  state->vs_dvp = parentdir;
  error = 0;

end:
  return error;
}

static void vnrstate_init(vnrstate_t *vs, vnrop_t op, const char *path) {
  vs->vs_op = op;
  vs->vs_cn.cn_flags = 0;
  vs->vs_nextcn = path;
}

int vfs_namelookup(const char *path, vnode_t **vp) {
  vnrstate_t vs;
  vnrstate_init(&vs, VNR_LOOKUP, path);
  int error = vfs_nameresolve(&vs);
  *vp = vs.vs_vp;
  return error;
}

int vfs_namecreate(const char *path, vnode_t **dvp, vnode_t **vp,
                   componentname_t *cn) {
  vnrstate_t vs;
  vnrstate_init(&vs, VNR_CREATE, path);
  int error = vfs_nameresolve(&vs);
  if (error)
    return error;

  *dvp = vs.vs_dvp;
  *vp = vs.vs_vp;
  memcpy(cn, &vs.vs_cn, sizeof(componentname_t));

  return 0;
}

int vfs_namedelete(const char *path, vnode_t **dvp, vnode_t **vp,
                   componentname_t *cn) {
  vnrstate_t vs;
  vnrstate_init(&vs, VNR_DELETE, path);
  int error = vfs_nameresolve(&vs);
  if (error)
    return error;

  *dvp = vs.vs_dvp;
  *vp = vs.vs_vp;
  memcpy(cn, &vs.vs_cn, sizeof(componentname_t));

  return 0;
}

int vfs_open(file_t *f, char *pathname, int flags, int mode) {
  vnode_t *v;
  int error = 0;
  if (flags & O_CREAT) {
    vnode_t *dvp;
    componentname_t cn;
    if ((error = vfs_namecreate(pathname, &dvp, &v, &cn)))
      return error;

    if (v == NULL) {
      vattr_t va;
      vattr_null(&va);
      va.va_mode = S_IFREG | (mode & ALLPERMS);
      error = VOP_CREATE(dvp, &cn, &va, &v);
      vnode_put(dvp);
      if (error)
        return error;
      flags &= ~O_TRUNC;
    } else {
      if (v == dvp)
        vnode_drop(dvp);
      else
        vnode_put(dvp);

      if (mode & O_EXCL)
        error = EEXIST;
      mode &= ~O_CREAT;
    }
  } else {
    if ((error = vfs_namelookup(pathname, &v)))
      return error;
  }

  if (!error && flags & O_TRUNC) {
    vattr_t va;
    vattr_null(&va);
    va.va_size = 0;
    error = VOP_SETATTR(v, &va);
  }

  if (!error)
    error = VOP_OPEN(v, flags, f);

  /* Drop our reference to v. We received it from vfs_namelookup, but we no
     longer need it - file f keeps its own reference to v after open. */
  vnode_drop(v);
  return error;
}

SYSINIT_ADD(vfs, vfs_init, NODEPS);
