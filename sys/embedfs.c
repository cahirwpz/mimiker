#include <embedfs.h>
#include <mount.h>
#include <vnode.h>
#include <stdc.h>
#include <errno.h>
#include <linker_set.h>

/* The list of all embed file entries */
typedef TAILQ_HEAD(, embedfs_entry) embedfs_file_list_t;
static embedfs_file_list_t embedfs_file_list =
  TAILQ_HEAD_INITIALIZER(embedfs_file_list);

static int embedfs_vnode_read(vnode_t *v, uio_t *uio) {
  embedfs_entry_t *fent = v->v_data;
  int error = uiomove(fent->start, fent->size, uio);
  if (error < 0)
    return -error;
  return 0;
}

static vnodeops_t embedfs_vnode_ops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_op_notsup,
  .v_read = embedfs_vnode_read,
  .v_write = vnode_op_notsup,
};

static embedfs_entry_t *embedfs_get_by_name(const char *name) {
  embedfs_entry_t *fent;
  TAILQ_FOREACH (fent, &embedfs_file_list, list)
    if (!strcmp(name, fent->name))
      return fent;
  return NULL;
}

static int embedfs_root_lookup(vnode_t *dir, const char *name, vnode_t **res) {
  assert(dir == dir->v_mount->mnt_data);

  embedfs_entry_t *fent = embedfs_get_by_name(name);
  if (!fent)
    return ENOENT;

  *res = fent->vnode;
  vnode_ref(*res);

  return 0;
}

static int embedfs_root_readdir(vnode_t *dir, uio_t *uio) {
  /* TODO: Implement. */
  return ENOTSUP;
}

static vnodeops_t embedfs_root_ops = {
  .v_lookup = embedfs_root_lookup,
  .v_readdir = embedfs_root_readdir,
  .v_open = vnode_op_notsup,
  .v_read = vnode_op_notsup,
  .v_write = vnode_op_notsup,
};

static int embedfs_mount(mount_t *m) {
  vnode_t *root = vnode_new(V_DIR, &embedfs_root_ops);
  root->v_mount = m;

  m->mnt_data = root;

  return 0;
}

static int embedfs_root(mount_t *m, vnode_t **v) {
  *v = m->mnt_data;
  vnode_ref(*v);
  return 0;
}

static int embedfs_init(vfsconf_t *vfc) {

  /* Gather file entries. */
  SET_DECLARE(embedfs_entries, embedfs_entry_t);
  embedfs_entry_t **ptr;
  SET_FOREACH(ptr, embedfs_entries) {
    embedfs_entry_t *p = *ptr;
    log("Registering embedded file %p with name %s", p->start, p->name);
    if (embedfs_get_by_name(p->name) != NULL)
      panic("Multiple files embedded in kernel image tried to register using "
            "the same name %s",
            p->name);
    /* We don't allocate/recycle vnodes properly yet, so - for now, allocate a
       singleton vnode per each embedded file. */
    vnode_t *v = vnode_new(V_REG, &embedfs_vnode_ops);
    v->v_data = p;
    p->vnode = v;
    TAILQ_INSERT_TAIL(&embedfs_file_list, p, list);
  }

  /* Mount embedfs at /embed. */
  /* TODO: This should actually happen somewhere else in the init process, much
   * later, and is configuration-dependent. */
  vfs_domount(vfc, vfs_root_embedfs_vnode);

  return 0;
}

static vfsops_t embedfs_vfsops = {.vfs_mount = embedfs_mount,
                                  .vfs_root = embedfs_root,
                                  .vfs_init = embedfs_init};

static vfsconf_t embedfs_conf = {.vfc_name = "embedfs",
                                 .vfc_vfsops = &embedfs_vfsops};

SET_ENTRY(vfsconf, embedfs_conf);

EMBED_FILE("prog.uelf", prog_uelf);
EMBED_FILE("misbehave", misbehave_uelf);
EMBED_FILE("fd_test", fd_test_uelf);
