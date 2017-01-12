#include <errno.h>
#include <common.h>
#include <malloc.h>
#include <vm.h>
#include <vm_map.h>
#include <stdc.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>
#include <string.h>
#include <cpio.h>
#include <mount.h>
#include <linker_set.h>

static MALLOC_DEFINE(mpool, "cpio mem_pool");

static vm_addr_t rd_start;
static vm_addr_t rd_size;
static stat_head_t initrd_head;
static vnodeops_t initrd_ops;

extern char *kenv_get(const char *key);

stat_head_t *initrd_get_headers()
{
    return &initrd_head;
}

vm_addr_t initrd_get_start()
{
    return rd_start;
}

vm_addr_t get_rd_size()
{
    return rd_size;
}

static int base_atoi(char *s, int n, int base)
{
    int res = 0;
    for(int i = 0; i < n; i++)
    {
        int add = (s[i] <= '9') ? s[i]-'0' : s[i]-'A'+10;
        res *= base;
        res += add;
    }
    return res;
}

static int get_name_padding(int offset)
{
    static int pad[4] = {2,1,0,3};
    return pad[offset%4];
}

static int get_file_padding(int offset)
{
    static int pad[4] = {0,3,2,1};
    return pad[offset%4];
}

void dump_cpio_stat(cpio_file_stat_t *stat)
{
    kprintf("c_magic: %o\n", stat->c_magic);
    kprintf("c_ino: %u\n", stat->c_ino);
    kprintf("c_mode: %d\n", stat->c_mode);
    kprintf("c_uid: %d\n", stat->c_uid);
    kprintf("c_gid: %d\n", stat->c_gid);
    kprintf("c_nlink: %zu\n", stat->c_nlink);
    kprintf("c_mtime: %ld\n", stat->c_mtime);
    kprintf("c_filesize: %ld\n", stat->c_filesize);
    kprintf("c_dev_maj: %ld\n", stat->c_dev_maj);
    kprintf("c_dev_min: %ld\n", stat->c_dev_min);
    kprintf("c_rdev_maj: %ld\n", stat->c_rdev_maj);
    kprintf("c_rdev_min: %ld\n", stat->c_rdev_min);
    kprintf("c_namesize: %zu\n", stat->c_namesize);
    kprintf("c_chksum: %u\n", (unsigned)stat->c_chksum);
    kprintf("c_name: %s\n", stat->c_name);
    kprintf("\n");
}

static void read_from_tape(char **tape, void *ptr, size_t bytes)
{
    memcpy(ptr, *tape, bytes);
    *tape += bytes;
}

static void skip_bytes(char **tape, size_t bytes)
{
    *tape += bytes;
}

static void fill_header(char **tape, cpio_file_stat_t *stat)
{
    cpio_hdr_t hdr;
    read_from_tape(tape, &hdr, sizeof(cpio_hdr_t));
    if(strncmp("070702", hdr.c_magic, 6) != 0)
    {
        kprintf("wrong magic number: ");
        for(int i = 0; i < 6; i++)
            kprintf("%c", hdr.c_magic[i]);
        kprintf("\n");
        panic();
    }

    stat->c_magic = base_atoi(hdr.c_magic, 6, 8);
    stat->c_ino = base_atoi(hdr.c_ino, 8, 16);
    stat->c_mode = base_atoi(hdr.c_mode, 8, 16);
    stat->c_uid = base_atoi(hdr.c_uid, 8, 16);
    stat->c_gid = base_atoi(hdr.c_gid, 8, 16);
    stat->c_nlink = base_atoi(hdr.c_nlink, 8, 16);
    stat->c_mtime = base_atoi(hdr.c_mtime, 8, 16);
    stat->c_filesize = base_atoi(hdr.c_filesize, 8, 16);
    stat->c_dev_maj = base_atoi(hdr.c_dev_maj, 8, 16);
    stat->c_dev_min = base_atoi(hdr.c_dev_min, 8, 16);
    stat->c_rdev_maj = base_atoi(hdr.c_rdev_maj, 8, 16);
    stat->c_rdev_min = base_atoi(hdr.c_rdev_min, 8, 16);
    stat->c_namesize = base_atoi(hdr.c_namesize, 8, 16);
    stat->c_chksum = base_atoi(hdr.c_chksum, 8, 16);

    stat->c_name = (char*)kmalloc(mpool, stat->c_namesize, 0);
    read_from_tape(tape, stat->c_name, stat->c_namesize);

    int pad = get_name_padding(stat->c_namesize);
    skip_bytes(tape, pad);
    stat->c_data = *tape;
    pad = get_file_padding(stat->c_filesize);
    skip_bytes(tape, pad+stat->c_filesize);
}

void cpio_init()
{

}

void initrd_collect_headers(stat_head_t *hd, char *tape)
{
    cpio_file_stat_t *stat;
    do 
    {
        stat = (cpio_file_stat_t*)kmalloc(mpool, sizeof(cpio_file_stat_t), M_ZERO);
        fill_header(&tape, stat);
        TAILQ_INSERT_TAIL(hd, stat, stat_list);
    } while(strcmp(stat->c_name, CPIO_TRAILER_NAME) != 0);
}

static int initrd_vnode_lookup(vnode_t *dir, const char *name, vnode_t **res)
{
    cpio_file_stat_t *it;
    stat_head_t *hd = initrd_get_headers();

    cpio_file_stat_t *dir_stat = (cpio_file_stat_t*)dir->v_data;
    int dir_name_len = strlen(dir_stat->c_name);
    int name_len = strlen(name);
    int buf_len = dir_name_len+name_len+2;
    char *buf = kmalloc(mpool, buf_len, 0);
    buf[0] = '\0';

    strlcat(buf, dir_stat->c_name, buf_len);
    if(strcmp(dir_stat->c_name, "") != 0) 
        strlcat(buf, "/", buf_len);
    strlcat(buf, name, buf_len);

    TAILQ_FOREACH(it, hd, stat_list)
    {
        if(strcmp(buf, it->c_name) == 0)
        {
            if(it->vnode == NULL)
            {
                vnodetype_t type = V_REG;
                if(it->c_mode & C_ISDIR)
                    type = V_DIR;
                *res = vnode_new(type, &initrd_ops);
                (*res)->v_data = (void*)it;
                it->vnode = *res;
                vnode_ref(*res);
            }
            else
                *res = it->vnode;
            vnode_ref(*res);
            kfree(mpool, buf);
            return 0;
        }
    }
    kfree(mpool, buf);
    return ENOENT;
}

static int initrd_vnode_read(vnode_t *v, uio_t *uio)
{
    cpio_file_stat_t *stat = v->v_data;
    int count = uio->uio_resid;
    int error = uiomove(stat->c_data, stat->c_filesize, uio);

    if (error < 0)
      return -error;

    return count - uio->uio_resid;
}

static int initrd_mount(mount_t *m)
{
    vnode_t *root = vnode_new(V_DIR, &initrd_ops);

    cpio_file_stat_t *stat = kmalloc(mpool, sizeof(cpio_file_stat_t), M_ZERO);

    char *buf = kmalloc(mpool, 1, M_ZERO);
    buf[0] = '\0';
    stat->c_name = buf;

    root->v_data = stat;

    root->v_mount = m;
    m->mnt_data = root;
    return 0;
}

static int initrd_root(mount_t *m, vnode_t **v)
{
    *v = m->mnt_data;
    vnode_ref(*v);
    return 0;
}

static int initrd_init(vfsconf_t *vfc)
{
    vfs_domount(vfc, vfs_root_vnode);
    return 0;
}

void ramdisk_init()
{
    char *s = kenv_get("rd_start");
    rd_start = s ? strtoul(s, NULL, 0) : 0;
    s = kenv_get("rd_size");
    rd_size = s ? align(strtoul(s, NULL, 0), PAGESIZE) : 0;

    TAILQ_INIT(&initrd_head);

    initrd_ops.v_lookup = vnode_op_notsup;
    initrd_ops.v_readdir = vnode_op_notsup;
    initrd_ops.v_open = vnode_op_notsup;
    initrd_ops.v_read =  vnode_op_notsup;
    initrd_ops.v_write = vnode_op_notsup;


    if(rd_size > 0)
    {
        initrd_ops.v_lookup = initrd_vnode_lookup;
        initrd_ops.v_read = initrd_vnode_read;
    }

    vm_page_t *pg = pm_alloc(2);
    kmalloc_init(mpool);
    kmalloc_add_arena(mpool, pg->vaddr, PG_SIZE(pg));

    if(rd_size > 0)
    {
        initrd_collect_headers(&initrd_head, (char*)initrd_get_start());
    }
}


vfsops_t initrd_vfsops = {
    .vfs_mount = initrd_mount,
    .vfs_root = initrd_root,
    .vfs_init = initrd_init
};

vfsconf_t initrd_conf = 
{
    .vfc_name = "initrd",
    .vfc_vfsops = &initrd_vfsops
};

SET_ENTRY(vfsconf, initrd_conf);
