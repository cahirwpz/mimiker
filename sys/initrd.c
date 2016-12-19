#include <common.h>
#include <malloc.h>
#include <vm.h>
#include <vm_map.h>
#include <stdc.h>
#include <initrd.h>
#include <vnode.h>
#include <mount.h>

static MALLOC_DEFINE(mpool, "cpio mem_pool");

static vm_addr_t rd_start;
static vm_addr_t rd_size;


vm_addr_t get_rd_start()
{
    return rd_start;
}

vm_addr_t get_rd_size()
{
    return rd_size;
}

int hex_atoi(char *s, int n)
{
    int res = 0;
    for(int i = 0; i < n; i++)
    {
        int add = (s[i] <= '9') ? s[i]-'0' : s[i]-'A'+10;
        res *= 16;
        res += add;
    }
    return res;
}

int oct_atoi(const char *s, int n)
{
    int res = 0;
    for(int i = 0; i < n; i++)
    {
        int add = s[i]-'0';
        res *= 8;
        res += add;
    }
    return res;
}

void ramdisk_init(vm_addr_t _rd_start, vm_addr_t _rd_size)
{
    rd_start = _rd_start;
    rd_size = _rd_size;
}

int get_name_padding(int offset)
{
    return (4 - (offset % 4));
}

int get_file_padding(int offset)
{
    return (4 - (offset % 4)) % 4;
}

void dump_cpio_stat(cpio_file_stat_t *stat)
{
    kprintf("c_magic: %o\n", stat->c_magic);
    kprintf("c_ino: %d\n", stat->c_ino);
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
    kprintf("c_namesize: %u\n", stat->c_namesize);
    kprintf("c_chksum: %u\n", (unsigned)stat->c_chksum);
    kprintf("c_name: %s\n", stat->c_name);
    kprintf("\n");
}

void read_from_tape(char **tape, void *ptr, size_t bytes)
{
    memcpy(ptr, *tape, bytes);
    *tape += bytes;
}

void skip_bytes(char **tape, size_t bytes)
{
    *tape += bytes;
}

void fill_header(char **tape, cpio_file_stat_t *stat)
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

    stat->c_magic = oct_atoi(hdr.c_magic, 6);
    stat->c_ino = hex_atoi(hdr.c_ino, 8);
    stat->c_mode = hex_atoi(hdr.c_mode, 8);
    stat->c_uid = hex_atoi(hdr.c_uid, 8);
    stat->c_gid = hex_atoi(hdr.c_gid, 8);
    stat->c_nlink = hex_atoi(hdr.c_nlink, 8);
    stat->c_mtime = hex_atoi(hdr.c_mtime, 8);
    stat->c_filesize = hex_atoi(hdr.c_filesize, 8);
    stat->c_dev_maj = hex_atoi(hdr.c_dev_maj, 8);
    stat->c_dev_min = hex_atoi(hdr.c_dev_min, 8);
    stat->c_rdev_maj = hex_atoi(hdr.c_rdev_maj, 8);
    stat->c_rdev_min = hex_atoi(hdr.c_rdev_min, 8);
    stat->c_namesize = hex_atoi(hdr.c_namesize, 8);
    stat->c_chksum = hex_atoi(hdr.c_chksum, 8);

    stat->c_name = (char*)kmalloc(mpool, stat->c_namesize, 0);
    read_from_tape(tape, stat->c_name, stat->c_namesize);

    /* Below are few magic moments, i have no idea why it works */

    int pad = get_name_padding(stat->c_namesize+1); /* +1 has to be here for some reason */
    skip_bytes(tape, pad);
    pad = get_file_padding(stat->c_filesize);
    skip_bytes(tape, pad+stat->c_filesize-1); /* -1 has to be here for some reason */
}


int initrd_vnode_lookup(vnode_t *dir, const char *name, vnode_t **res)
{
    return 0;
}

void cpio_init()
{
    vm_page_t *pg = pm_alloc(2);
    kmalloc_init(mpool);
    kmalloc_add_arena(mpool, pg->vaddr, PG_SIZE(pg));
}

//
//static int initrd_vnode_readdir(vnode_t *v, uio_t *uio)
//{
//    return 0;
//}
//
//static int initrd_vnode_open(vnode_t *v, int mode, file_t *fp)
//{
//    return 0;
//}
//
//static int initrd_vnode_read(vnode_t *v, uio_t *uio)
//{
//    return 0;
//}
//
//int initrd_mount()
//{
//    return 0;
//}
//
//int initrd_root()
//{
//    return 0;
//}

//static vnodeops_t initrd_ops = 
//{
//  .v_lookup = initrd_vnode_lookup,
//  .v_readdir = initrd_vnode_readdir,
//  .v_open = initrd_vnode_open,
//  .v_read = initrd_vnode_read,
//  .v_write = vnode_op_notsup
//};

//static vfsops_t initrd_vfsops = {.vfs_mount = initrd_mount,
//                                  .vfs_root = initrd_root};
//                                  

//static vfsconf_t initrd_conf = 
//{
//    .vfc_name = "initrd",
//    .vfc_vfsops = &initrd_vfsops
//};

