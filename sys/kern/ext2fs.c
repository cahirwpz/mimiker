#define KL_LOG KL_FILESYS
#include <sys/mimiker.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/types.h>

static KMALLOC_DEFINE(M_EXT2, "ext2fs");

/*
 * The first super block and block group descriptors offsets are given in
 * absolute disk addresses.
 */
#define EXT2_SBOFF ((off_t)1024)
#define EXT2_GDOFF ((off_t)2048)

#define EXT2_BLKSIZE 1024

#define EXT2_NDADDR 12 /* Direct addresses in inode. */
#define EXT2_NIADDR 3  /* Indirect addresses in inode. */
#define EXT2_NADDR (EXT2_NDADDR + EXT2_NIADDR)

/* How many block pointers fit into one block? */
#define EXT2_BLK_POINTERS (EXT2_BLKSIZE / sizeof(uint32_t))

/*
 * File system super block.
 */
typedef struct ext2_superblock {
  uint32_t sb_icount;        /* Inode count */
  uint32_t sb_bcount;        /* blocks count */
  uint32_t sb_rbcount;       /* reserved blocks count */
  uint32_t sb_fbcount;       /* free blocks count */
  uint32_t sb_ficount;       /* free inodes count */
  uint32_t sb_first_dblock;  /* first data block */
  uint32_t sb_log_bsize;     /* bsize = 1024*(2^sb_log_bsize) */
  uint32_t sb_fsize;         /* fragment size */
  uint32_t sb_bpg;           /* blocks per group */
  uint32_t sb_fpg;           /* frags per group */
  uint32_t sb_ipg;           /* inodes per group */
  uint32_t sb_mtime;         /* mount time */
  uint32_t sb_wtime;         /* write time */
  uint16_t sb_mnt_count;     /* mount count */
  uint16_t sb_max_mnt_count; /* max mount count */
  uint16_t sb_magic;         /* magic number */
  uint16_t sb_state;         /* file system state */
  uint16_t sb_beh;           /* behavior on errors */
  uint16_t sb_minrev;        /* minor revision level */
  uint32_t sb_lastfsck;      /* time of last fsck */
  uint32_t sb_fsckintv;      /* max time between fscks */
  uint32_t sb_creator;       /* creator OS */
  uint32_t sb_rev;           /* revision level */
  uint16_t sb_ruid;          /* default uid for reserved blocks */
  uint16_t sb_rgid;          /* default gid for reserved blocks */
  /* EXT2_DYNAMIC_REV superblocks */
  uint32_t sb_first_ino;         /* first non-reserved inode */
  uint16_t sb_inode_size;        /* size of inode structure */
  uint16_t sb_block_group_nr;    /* block grp number of this sblk */
  uint32_t sb_features_compat;   /*  compatible feature set */
  uint32_t sb_features_incompat; /* incompatible feature set */
  uint32_t sb_features_rocompat; /* RO-compatible feature set */
  uint8_t sb_uuid[16];           /* 128-bit uuid for volume */
  char sb_vname[16];             /* volume name */
  char sb_fsmnt[64];             /* name mounted on */
  uint32_t sb_algo;              /* For compression */
  uint8_t sb_prealloc;           /* # of blocks to preallocate */
  uint8_t sb_dir_prealloc;       /* # of blocks to preallocate for dir */
  uint16_t sb_reserved_ngdb;     /* # of reserved gd blocks for resize */
} ext2_superblock_t;

/*
 * Structure of an inode on the disk.
 */
typedef struct ext2_inode {
  uint16_t i_mode;               /*   0: IFMT, permissions; see below. */
  uint16_t i_uid;                /*   2: Owner UID */
  uint32_t i_size;               /*   4: Size (in bytes) */
  uint32_t i_atime;              /*   8: Access time */
  uint32_t i_ctime;              /*  12: Change time */
  uint32_t i_mtime;              /*  16: Modification time */
  uint32_t i_dtime;              /*  20: Deletion time */
  uint16_t i_gid;                /*  24: Owner GID */
  uint16_t i_nlink;              /*  26: File link count */
  uint32_t i_nblock;             /*  28: Blocks count */
  uint32_t i_flags;              /*  32: Status flags (chflags) */
  uint32_t i_version;            /*  36: Low 32 bits inode version */
  uint32_t i_blocks[EXT2_NADDR]; /*  40: disk blocks */
  uint32_t i_gen;                /* 100: generation number */
  uint32_t i_facl;               /* 104: Low EA block */
  uint32_t i_size_high;          /* 108: Upper bits of file size */
  uint32_t i_faddr;              /* 112: Fragment address (obsolete) */
  uint16_t i_nblock_high;        /* 116: Blocks count bits 47:32 */
  uint16_t i_facl_high;          /* 118: File EA bits 47:32 */
  uint16_t i_uid_high;           /* 120: Owner UID top 16 bits */
  uint16_t i_gid_high;           /* 122: Owner GID top 16 bits */
  uint16_t i_chksum_lo;          /* 124: Lower inode checksum */
  uint16_t i_lx_reserved;        /* 126: Unused */
} ext2_inode_t;

/* Reads i-node bitmap entry for `ino`. Returns 0 if the i-node is free,
 * 1 if it's in use, and EINVAL if `ino` value is out of range. */
int ext2_inode_used(uint32_t ino) {
  if (!ino || ino >= inode_count)
    return EINVAL;
  int used = 0;
#ifndef STUDENT
  size_t gdidx = (ino - 1) / inodes_per_group;
  size_t diidx = (ino - 1) % inodes_per_group;
  blk_t *blk = blk_get(0, group_desc[gdidx].gd_i_bitmap);
  /*
   * The first block of this block group is represented by bit 0 of byte 0,
   * the second by bit 1 of byte 0. The 8th block is represented by bit 7
   * (most significant bit) of byte 0 while the 9th block is represented
   * by bit 0 (least significant bit) of byte 1.
   */
  uint8_t *ibm = blk->b_data;
  used = (ibm[diidx / 8] >> (diidx % 8)) & 1;
  blk_put(blk);
#endif /* STUDENT */
  return used;
}

/* Reads i-node identified by number `ino`.
 * Returns 0 on success. If i-node is not allocated returns ENOENT. */
static int ext2_inode_read(off_t ino, ext2_inode_t *inode) {
#ifndef STUDENT
  if (!ext2_inode_used(ino))
    return ENOENT;

  size_t gdidx = (ino - 1) / inodes_per_group;
  size_t diidx = (ino - 1) % inodes_per_group;
  blk_t *blk = blk_get(0, group_desc[gdidx].gd_i_tables + diidx / BLK_INODES);
  memcpy(inode, blk->b_data + (diidx % BLK_INODES) * sizeof(ext2_inode_t),
         sizeof(ext2_inode_t));
  blk_put(blk);
  return 0;
#endif /* STUDENT */
  return ENOENT;
}

long ext2_blkaddr_read(uint32_t ino, uint32_t blkidx) {
  /* No translation for filesystem metadata blocks. */
  if (ino == 0)
    return blkidx;

  ext2_inode_t inode;
  if (ext2_inode_read(ino, &inode))
    return -1;

  /* Read direct pointers or pointers from indirect blocks. */
  /* Check if `blkidx` is not past the end of file. */
  if (blkidx * EXT2_BLKSIZE >= inode.i_size)
    return -1;

  if (blkidx < EXT2_NDADDR)
    return inode.i_blocks[blkidx];

  blkidx -= EXT2_NDADDR;
  if (blkidx < EXT2_BLK_POINTERS)
    return ext2_blkptr_read(inode.i_blocks[EXT2_NDADDR], blkidx);

  uint32_t l2blkaddr, l3blkaddr, l1blkidx;

  blkidx -= EXT2_BLK_POINTERS;
  if (blkidx < EXT2_BLK_POINTERS * EXT2_BLK_POINTERS) {
    l2blkaddr =
      ext2_blkptr_read(inode.i_blocks[EXT2_NDADDR + 1],
                       blkidx / EXT2_BLK_POINTERS);
    return ext2_blkptr_read(l2blkaddr, blkidx % EXT2_BLK_POINTERS);
  }

  blkidx -= EXT2_BLK_POINTERS * EXT2_BLK_POINTERS;
  l1blkidx = blkidx / (EXT2_BLK_POINTERS * EXT2_BLK_POINTERS);
  blkidx %= EXT2_BLK_POINTERS * EXT2_BLK_POINTERS;
  l2blkaddr = ext2_blkptr_read(inode.i_blocks[EXT2_NDADDR + 2], l1blkidx);
  l3blkaddr = ext2_blkptr_read(l2blkaddr, blkidx / EXT2_BLK_POINTERS);
  return ext2_blkptr_read(l3blkaddr, blkidx % EXT2_BLK_POINTERS);
  return -1;
}

static int ext2_read(ino_t ino, void *data, size_t pos, size_t len) {
  long off = pos % EXT2_BLKSIZE;
  long idx = pos / EXT2_BLKSIZE;

  while (len > 0) {
    size_t cnt = min(EXT2_BLKSIZE - off, len);
    /* void *blk = blk_get(ino, idx++);
    if (blk == NULL)
      return EINVAL;
    if (blk == BLK_ZERO) {
      memset(data, 0, cnt);
    } else {
      memcpy(data, blk->b_data + off, cnt);
      blk_put(blk);
    } */
    data += cnt;
    len -= cnt;
    off = 0;
  }
}

static int ext2_mount(mount_t *mp) {
  int error;

  ext2_superblock_t *sb = kmalloc(M_EXT2, sizeof(ext2_superblock_t), 0);

  
}

static int ext2_root(mount_t *mp, vnode_t **vp) {

}

static int ext2_init(vfsconf_t *vfc) {

}

static vfsops_t ext2_vfsops = {
  .vfs_mount = ext2_mount,
  .vfs_root = ext2_root,
  .vfs_init = ext2_init,
};

static vfsconf_t ext2_conf = {
  .vfc_name = "ext2",
  .vfc_vfsops = &ext2_vfsops,
};
