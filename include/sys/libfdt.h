#ifndef _SYS_LIBFDT_H_
#define _SYS_LIBFDT_H_
/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 *
 * libfdt is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 *
 *  a) This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of the
 *     License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this library; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *     MA 02110-1301 USA
 *
 * Alternatively,
 *
 *  b) Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *     1. Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *     OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *     EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>

typedef uint16_t fdt16_t;
typedef uint32_t fdt32_t;
typedef uint64_t fdt64_t;

struct fdt_header {
  fdt32_t magic;             /* magic word FDT_MAGIC */
  fdt32_t totalsize;         /* total size of DT block */
  fdt32_t off_dt_struct;     /* offset to structure */
  fdt32_t off_dt_strings;    /* offset to strings */
  fdt32_t off_mem_rsvmap;    /* offset to memory reserve map */
  fdt32_t version;           /* format version */
  fdt32_t last_comp_version; /* last compatible version */

  /* version 2 fields below */
  fdt32_t boot_cpuid_phys; /* Which physical CPU id we're
                              booting on */
  /* version 3 fields below */
  fdt32_t size_dt_strings; /* size of the strings block */

  /* version 17 fields below */
  fdt32_t size_dt_struct; /* size of the structure block */
};

struct fdt_reserve_entry {
  fdt64_t address;
  fdt64_t size;
};

struct fdt_node_header {
  fdt32_t tag;
  char name[0];
};

struct fdt_property {
  fdt32_t tag;
  fdt32_t len;
  fdt32_t nameoff;
  char data[0];
};

#define FDT_MAGIC 0xd00dfeed /* 4: version, 4: total size */
#define FDT_TAGSIZE sizeof(fdt32_t)

#define FDT_BEGIN_NODE 0x1 /* Start node: full name */
#define FDT_END_NODE 0x2   /* End node */
#define FDT_PROP                                                               \
  0x3               /* Property: name off,                                     \
                       size, content */
#define FDT_NOP 0x4 /* nop */
#define FDT_END 0x9

#define FDT_V1_SIZE (7 * sizeof(fdt32_t))
#define FDT_V2_SIZE (FDT_V1_SIZE + sizeof(fdt32_t))
#define FDT_V3_SIZE (FDT_V2_SIZE + sizeof(fdt32_t))
#define FDT_V16_SIZE FDT_V3_SIZE
#define FDT_V17_SIZE (FDT_V16_SIZE + sizeof(fdt32_t))

#define FDT_FIRST_SUPPORTED_VERSION 0x02
#define FDT_LAST_SUPPORTED_VERSION 0x11

/* Error codes: informative error codes */
#define FDT_ERR_NOTFOUND 1
/* FDT_ERR_NOTFOUND: The requested node or property does not exist */
#define FDT_ERR_EXISTS 2
/* FDT_ERR_EXISTS: Attempted to create a node or property which
 * already exists */
#define FDT_ERR_NOSPACE 3
/* FDT_ERR_NOSPACE: Operation needed to expand the device
 * tree, but its buffer did not have sufficient space to
 * contain the expanded tree. Use fdt_open_into() to move the
 * device tree to a buffer with more space. */

/* Error codes: codes for bad parameters */
#define FDT_ERR_BADOFFSET 4
/* FDT_ERR_BADOFFSET: Function was passed a structure block
 * offset which is out-of-bounds, or which points to an
 * unsuitable part of the structure for the operation. */
#define FDT_ERR_BADPATH 5
/* FDT_ERR_BADPATH: Function was passed a badly formatted path
 * (e.g. missing a leading / for a function which requires an
 * absolute path) */
#define FDT_ERR_BADPHANDLE 6
/* FDT_ERR_BADPHANDLE: Function was passed an invalid phandle.
 * This can be caused either by an invalid phandle property
 * length, or the phandle value was either 0 or -1, which are
 * not permitted. */
#define FDT_ERR_BADSTATE 7
/* FDT_ERR_BADSTATE: Function was passed an incomplete device
 * tree created by the sequential-write functions, which is
 * not sufficiently complete for the requested operation. */

/* Error codes: codes for bad device tree blobs */
#define FDT_ERR_TRUNCATED 8
/* FDT_ERR_TRUNCATED: Structure block of the given device tree
 * ends without an FDT_END tag. */
#define FDT_ERR_BADMAGIC 9
/* FDT_ERR_BADMAGIC: Given "device tree" appears not to be a
 * device tree at all - it is missing the flattened device
 * tree magic number. */
#define FDT_ERR_BADVERSION 10
/* FDT_ERR_BADVERSION: Given device tree has a version which
 * can't be handled by the requested operation.  For
 * read-write functions, this may mean that fdt_open_into() is
 * required to convert the tree to the expected version. */
#define FDT_ERR_BADSTRUCTURE 11
/* FDT_ERR_BADSTRUCTURE: Given device tree has a corrupt
 * structure block or other serious error (e.g. misnested
 * nodes, or subnodes preceding properties). */
#define FDT_ERR_BADLAYOUT 12
/* FDT_ERR_BADLAYOUT: For read-write functions, the given
 * device tree has it's sub-blocks in an order that the
 * function can't handle (memory reserve map, then structure,
 * then strings).  Use fdt_open_into() to reorganize the tree
 * into a form suitable for the read-write operations. */

/* "Can't happen" error indicating a bug in libfdt */
#define FDT_ERR_INTERNAL 13
/* FDT_ERR_INTERNAL: libfdt has failed an internal assertion.
 * Should never be returned, if it is, it indicates a bug in
 * libfdt itself. */

/* Errors in device tree content */
#define FDT_ERR_BADNCELLS 14
/* FDT_ERR_BADNCELLS: Device tree has a #address-cells, #size-cells
 * or similar property with a bad format or value */

#define FDT_ERR_BADVALUE 15
/* FDT_ERR_BADVALUE: Device tree has a property with an unexpected
 * value. For example: a property expected to contain a string list
 * is not NUL-terminated within the length of its value. */

#define FDT_ERR_BADOVERLAY 16
/* FDT_ERR_BADOVERLAY: The device tree overlay, while
 * correctly structured, cannot be applied due to some
 * unexpected or missing value, property or node. */

#define FDT_ERR_NOPHANDLES 17
/* FDT_ERR_NOPHANDLES: The device tree doesn't have any
 * phandle available anymore without causing an overflow */

#define FDT_ERR_MAX 17

#define EXTRACT_BYTE(x, n) ((unsigned long long)((uint8_t *)&x)[n])
#define CPU_TO_FDT16(x) ((EXTRACT_BYTE(x, 0) << 8) | EXTRACT_BYTE(x, 1))
#define CPU_TO_FDT32(x)                                                        \
  ((EXTRACT_BYTE(x, 0) << 24) | (EXTRACT_BYTE(x, 1) << 16) |                   \
   (EXTRACT_BYTE(x, 2) << 8) | EXTRACT_BYTE(x, 3))
#define CPU_TO_FDT64(x)                                                        \
  ((EXTRACT_BYTE(x, 0) << 56) | (EXTRACT_BYTE(x, 1) << 48) |                   \
   (EXTRACT_BYTE(x, 2) << 40) | (EXTRACT_BYTE(x, 3) << 32) |                   \
   (EXTRACT_BYTE(x, 4) << 24) | (EXTRACT_BYTE(x, 5) << 16) |                   \
   (EXTRACT_BYTE(x, 6) << 8) | EXTRACT_BYTE(x, 7))

static inline uint16_t fdt16_to_cpu(fdt16_t x) {
  return (uint16_t)CPU_TO_FDT16(x);
}
static inline fdt16_t cpu_to_fdt16(uint16_t x) {
  return (fdt16_t)CPU_TO_FDT16(x);
}

static inline uint32_t fdt32_to_cpu(fdt32_t x) {
  return (uint32_t)CPU_TO_FDT32(x);
}
static inline fdt32_t cpu_to_fdt32(uint32_t x) {
  return (fdt32_t)CPU_TO_FDT32(x);
}

static inline uint64_t fdt64_to_cpu(fdt64_t x) {
  return (uint64_t)CPU_TO_FDT64(x);
}
static inline fdt64_t cpu_to_fdt64(uint64_t x) {
  return (fdt64_t)CPU_TO_FDT64(x);
}
#undef CPU_TO_FDT64
#undef CPU_TO_FDT32
#undef CPU_TO_FDT16
#undef EXTRACT_BYTE

/**********************************************************************/
/* Low-level functions (you probably don't need these)                */
/**********************************************************************/

const void *fdt_offset_ptr(const void *fdt, int offset, unsigned int checklen);

uint32_t fdt_next_tag(const void *fdt, int offset, int *nextoffset);

/**********************************************************************/
/* Traversal functions                                                */
/**********************************************************************/

int fdt_next_node(const void *fdt, int offset, int *depth);

/**
 * fdt_first_subnode() - get offset of first direct subnode
 *
 * @fdt:	FDT blob
 * @offset:	Offset of node to check
 * @return offset of first subnode, or -FDT_ERR_NOTFOUND if there is none
 */
int fdt_first_subnode(const void *fdt, int offset);

/**
 * fdt_next_subnode() - get offset of next direct subnode
 *
 * After first calling fdt_first_subnode(), call this function repeatedly to
 * get direct subnodes of a parent node.
 *
 * @fdt:	FDT blob
 * @offset:	Offset of previous subnode
 * @return offset of next subnode, or -FDT_ERR_NOTFOUND if there are no more
 * subnodes
 */
int fdt_next_subnode(const void *fdt, int offset);

/**
 * fdt_for_each_subnode - iterate over all subnodes of a parent
 *
 * @node:	child node (int, lvalue)
 * @fdt:	FDT blob (const void *)
 * @parent:	parent node (int)
 *
 * This is actually a wrapper around a for loop and would be used like so:
 *
 *	fdt_for_each_subnode(node, fdt, parent) {
 *		Use node
 *		...
 *	}
 *
 *	if ((node < 0) && (node != -FDT_ERR_NOT_FOUND)) {
 *		Error handling
 *	}
 *
 * Note that this is implemented as a macro and @node is used as
 * iterator in the loop. The parent variable be constant or even a
 * literal.
 *
 */
#define fdt_for_each_subnode(node, fdt, parent)                                \
  for (node = fdt_first_subnode(fdt, parent); node >= 0;                       \
       node = fdt_next_subnode(fdt, node))

/**********************************************************************/
/* General functions                                                  */
/**********************************************************************/
#define fdt_get_header(fdt, field)                                             \
  (fdt32_to_cpu(((const struct fdt_header *)(fdt))->field))
#define fdt_magic(fdt) (fdt_get_header(fdt, magic))
#define fdt_totalsize(fdt) (fdt_get_header(fdt, totalsize))
#define fdt_off_dt_struct(fdt) (fdt_get_header(fdt, off_dt_struct))
#define fdt_off_dt_strings(fdt) (fdt_get_header(fdt, off_dt_strings))
#define fdt_off_mem_rsvmap(fdt) (fdt_get_header(fdt, off_mem_rsvmap))
#define fdt_version(fdt) (fdt_get_header(fdt, version))
#define fdt_last_comp_version(fdt) (fdt_get_header(fdt, last_comp_version))
#define fdt_boot_cpuid_phys(fdt) (fdt_get_header(fdt, boot_cpuid_phys))
#define fdt_size_dt_strings(fdt) (fdt_get_header(fdt, size_dt_strings))
#define fdt_size_dt_struct(fdt) (fdt_get_header(fdt, size_dt_struct))

#define fdt_set_hdr_(name)                                                     \
  static inline void fdt_set_##name(void *fdt, uint32_t val) {                 \
    struct fdt_header *fdth = (struct fdt_header *)fdt;                        \
    fdth->name = cpu_to_fdt32(val);                                            \
  }
fdt_set_hdr_(magic);
fdt_set_hdr_(totalsize);
fdt_set_hdr_(off_dt_struct);
fdt_set_hdr_(off_dt_strings);
fdt_set_hdr_(off_mem_rsvmap);
fdt_set_hdr_(version);
fdt_set_hdr_(last_comp_version);
fdt_set_hdr_(boot_cpuid_phys);
fdt_set_hdr_(size_dt_strings);
fdt_set_hdr_(size_dt_struct);
#undef fdt_set_hdr_

/**
 * fdt_check_header - sanity check a device tree or possible device tree
 * @fdt: pointer to data which might be a flattened device tree
 *
 * fdt_check_header() checks that the given buffer contains what
 * appears to be a flattened device tree with sane information in its
 * header.
 *
 * returns:
 *     0, if the buffer appears to contain a valid device tree
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE, standard meanings, as above
 */
int fdt_check_header(const void *fdt);

/**********************************************************************/
/* Read-only functions                                                */
/**********************************************************************/

/**
 * fdt_string - retrieve a string from the strings block of a device tree
 * @fdt: pointer to the device tree blob
 * @stroffset: offset of the string within the strings block (native endian)
 *
 * fdt_string() retrieves a pointer to a single string from the
 * strings block of the device tree blob at fdt.
 *
 * returns:
 *     a pointer to the string, on success
 *     NULL, if stroffset is out of bounds
 */
const char *fdt_string(const void *fdt, int stroffset);

/**
 * fdt_get_max_phandle - retrieves the highest phandle in a tree
 * @fdt: pointer to the device tree blob
 *
 * fdt_get_max_phandle retrieves the highest phandle in the given
 * device tree. This will ignore badly formatted phandles, or phandles
 * with a value of 0 or -1.
 *
 * returns:
 *      the highest phandle on success
 *      0, if no phandle was found in the device tree
 *      -1, if an error occurred
 */
uint32_t fdt_get_max_phandle(const void *fdt);

/**
 * fdt_num_mem_rsv - retrieve the number of memory reserve map entries
 * @fdt: pointer to the device tree blob
 *
 * Returns the number of entries in the device tree blob's memory
 * reservation map.  This does not include the terminating 0,0 entry
 * or any other (0,0) entries reserved for expansion.
 *
 * returns:
 *     the number of entries
 */
int fdt_num_mem_rsv(const void *fdt);

/**
 * fdt_get_mem_rsv - retrieve one memory reserve map entry
 * @fdt: pointer to the device tree blob
 * @address, @size: pointers to 64-bit variables
 *
 * On success, *address and *size will contain the address and size of
 * the n-th reserve map entry from the device tree blob, in
 * native-endian format.
 *
 * returns:
 *     0, on success
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE, standard meanings
 */
int fdt_get_mem_rsv(const void *fdt, int n, uint64_t *address, uint64_t *size);

/**
 * fdt_subnode_offset_namelen - find a subnode based on substring
 * @fdt: pointer to the device tree blob
 * @parentoffset: structure block offset of a node
 * @name: name of the subnode to locate
 * @namelen: number of characters of name to consider
 *
 * Identical to fdt_subnode_offset(), but only examine the first
 * namelen characters of name for matching the subnode name.  This is
 * useful for finding subnodes based on a portion of a larger string,
 * such as a full path.
 */
int fdt_subnode_offset_namelen(const void *fdt, int parentoffset,
                               const char *name, int namelen);

/**
 * fdt_subnode_offset - find a subnode of a given node
 * @fdt: pointer to the device tree blob
 * @parentoffset: structure block offset of a node
 * @name: name of the subnode to locate
 *
 * fdt_subnode_offset() finds a subnode of the node at structure block
 * offset parentoffset with the given name.  name may include a unit
 * address, in which case fdt_subnode_offset() will find the subnode
 * with that unit address, or the unit address may be omitted, in
 * which case fdt_subnode_offset() will find an arbitrary subnode
 * whose name excluding unit address matches the given name.
 *
 * returns:
 *	structure block offset of the requested subnode (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the requested subnode does not exist
 *	-FDT_ERR_BADOFFSET, if parentoffset did not point to an FDT_BEGIN_NODE
 *		tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_subnode_offset(const void *fdt, int parentoffset, const char *name);

/**
 * fdt_path_offset_namelen - find a tree node by its full path
 * @fdt: pointer to the device tree blob
 * @path: full path of the node to locate
 * @namelen: number of characters of path to consider
 *
 * Identical to fdt_path_offset(), but only consider the first namelen
 * characters of path as the path name.
 */
int fdt_path_offset_namelen(const void *fdt, const char *path, int namelen);

/**
 * fdt_path_offset - find a tree node by its full path
 * @fdt: pointer to the device tree blob
 * @path: full path of the node to locate
 *
 * fdt_path_offset() finds a node of a given path in the device tree.
 * Each path component may omit the unit address portion, but the
 * results of this are undefined if any such path component is
 * ambiguous (that is if there are multiple nodes at the relevant
 * level matching the given component, differentiated only by unit
 * address).
 *
 * returns:
 *	structure block offset of the node with the requested path (>=0), on
 *		success
 *	-FDT_ERR_BADPATH, given path does not begin with '/' or is invalid
 *	-FDT_ERR_NOTFOUND, if the requested node does not exist
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_path_offset(const void *fdt, const char *path);

/**
 * fdt_get_name - retrieve the name of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of the starting node
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_get_name() retrieves the name (including unit address) of the
 * device tree node at structure block offset nodeoffset.  If lenp is
 * non-NULL, the length of this name is also returned, in the integer
 * pointed to by lenp.
 *
 * returns:
 *	pointer to the node's name, on success
 *		If lenp is non-NULL, *lenp contains the length of that name
 *			(>=0)
 *	NULL, on error
 *		if lenp is non-NULL *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE
 *			tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE, standard meanings
 */
const char *fdt_get_name(const void *fdt, int nodeoffset, int *lenp);

/**
 * fdt_first_property_offset - find the offset of a node's first property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of a node
 *
 * fdt_first_property_offset() finds the first property of the node at
 * the given structure block offset.
 *
 * returns:
 *	structure block offset of the property (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the requested node has no properties
 *	-FDT_ERR_BADOFFSET, if nodeoffset did not point to an FDT_BEGIN_NODE tag
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_first_property_offset(const void *fdt, int nodeoffset);

/**
 * fdt_next_property_offset - step through a node's properties
 * @fdt: pointer to the device tree blob
 * @offset: structure block offset of a property
 *
 * fdt_next_property_offset() finds the property immediately after the
 * one at the given structure block offset.  This will be a property
 * of the same node as the given property.
 *
 * returns:
 *	structure block offset of the next property (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the given property is the last in its node
 *	-FDT_ERR_BADOFFSET, if nodeoffset did not point to an FDT_PROP tag
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_next_property_offset(const void *fdt, int offset);

/**
 * fdt_for_each_property_offset - iterate over all properties of a node
 *
 * @property_offset:	property offset (int, lvalue)
 * @fdt:		FDT blob (const void *)
 * @node:		node offset (int)
 *
 * This is actually a wrapper around a for loop and would be used like so:
 *
 *	fdt_for_each_property_offset(property, fdt, node) {
 *		Use property
 *		...
 *	}
 *
 *	if ((property < 0) && (property != -FDT_ERR_NOT_FOUND)) {
 *		Error handling
 *	}
 *
 * Note that this is implemented as a macro and property is used as
 * iterator in the loop. The node variable can be constant or even a
 * literal.
 */
#define fdt_for_each_property_offset(property, fdt, node)                      \
  for (property = fdt_first_property_offset(fdt, node); property >= 0;         \
       property = fdt_next_property_offset(fdt, property))

/**
 * fdt_get_property_by_offset - retrieve the property at a given offset
 * @fdt: pointer to the device tree blob
 * @offset: offset of the property to retrieve
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_get_property_by_offset() retrieves a pointer to the
 * fdt_property structure within the device tree blob at the given
 * offset.  If lenp is non-NULL, the length of the property value is
 * also returned, in the integer pointed to by lenp.
 *
 * Note that this code only works on device tree versions >= 16. fdt_getprop()
 * works on all versions.
 *
 * returns:
 *	pointer to the structure representing the property
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_PROP tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const struct fdt_property *fdt_get_property_by_offset(const void *fdt,
                                                      int offset, int *lenp);

/**
 * fdt_get_property_namelen - find a property based on substring
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @namelen: number of characters of name to consider
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * Identical to fdt_get_property(), but only examine the first namelen
 * characters of name for matching the property name.
 */
const struct fdt_property *fdt_get_property_namelen(const void *fdt,
                                                    int nodeoffset,
                                                    const char *name,
                                                    int namelen, int *lenp);

/**
 * fdt_get_property - find a given property in a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_get_property() retrieves a pointer to the fdt_property
 * structure within the device tree blob corresponding to the property
 * named 'name' of the node at offset nodeoffset.  If lenp is
 * non-NULL, the length of the property value is also returned, in the
 * integer pointed to by lenp.
 *
 * returns:
 *	pointer to the structure representing the property
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_NOTFOUND, node does not have named property
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE
 *			tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const struct fdt_property *fdt_get_property(const void *fdt, int nodeoffset,
                                            const char *name, int *lenp);

/**
 * fdt_getprop_by_offset - retrieve the value of a property at a given offset
 * @fdt: pointer to the device tree blob
 * @ffset: offset of the property to read
 * @namep: pointer to a string variable (will be overwritten) or NULL
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_getprop_by_offset() retrieves a pointer to the value of the
 * property at structure block offset 'offset' (this will be a pointer
 * to within the device blob itself, not a copy of the value).  If
 * lenp is non-NULL, the length of the property value is also
 * returned, in the integer pointed to by lenp.  If namep is non-NULL,
 * the property's namne will also be returned in the char * pointed to
 * by namep (this will be a pointer to within the device tree's string
 * block, not a new copy of the name).
 *
 * returns:
 *	pointer to the property's value
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *		if namep is non-NULL *namep contiains a pointer to the property
 *		name.
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_PROP tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const void *fdt_getprop_by_offset(const void *fdt, int offset,
                                  const char **namep, int *lenp);

/**
 * fdt_getprop_namelen - get property value based on substring
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @namelen: number of characters of name to consider
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * Identical to fdt_getprop(), but only examine the first namelen
 * characters of name for matching the property name.
 */
const void *fdt_getprop_namelen(const void *fdt, int nodeoffset,
                                const char *name, int namelen, int *lenp);

/**
 * fdt_getprop - retrieve the value of a given property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_getprop() retrieves a pointer to the value of the property
 * named 'name' of the node at offset nodeoffset (this will be a
 * pointer to within the device blob itself, not a copy of the value).
 * If lenp is non-NULL, the length of the property value is also
 * returned, in the integer pointed to by lenp.
 *
 * returns:
 *	pointer to the property's value
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_NOTFOUND, node does not have named property
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE
 *			tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const void *fdt_getprop(const void *fdt, int nodeoffset, const char *name,
                        int *lenp);

/**
 * fdt_get_phandle - retrieve the phandle of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of the node
 *
 * fdt_get_phandle() retrieves the phandle of the device tree node at
 * structure block offset nodeoffset.
 *
 * returns:
 *	the phandle of the node at nodeoffset, on success (!= 0, != -1)
 *	0, if the node has no phandle, or another error occurs
 */
uint32_t fdt_get_phandle(const void *fdt, int nodeoffset);

/**
 * fdt_get_alias_namelen - get alias based on substring
 * @fdt: pointer to the device tree blob
 * @name: name of the alias th look up
 * @namelen: number of characters of name to consider
 *
 * Identical to fdt_get_alias(), but only examine the first namelen
 * characters of name for matching the alias name.
 */
const char *fdt_get_alias_namelen(const void *fdt, const char *name,
                                  int namelen);

/**
 * fdt_get_alias - retrieve the path referenced by a given alias
 * @fdt: pointer to the device tree blob
 * @name: name of the alias th look up
 *
 * fdt_get_alias() retrieves the value of a given alias.  That is, the
 * value of the property named 'name' in the node /aliases.
 *
 * returns:
 *	a pointer to the expansion of the alias named 'name', if it exists
 *	NULL, if the given alias or the /aliases node does not exist
 */
const char *fdt_get_alias(const void *fdt, const char *name);

/**
 * fdt_get_path - determine the full path of a node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose path to find
 * @buf: character buffer to contain the returned path (will be overwritten)
 * @buflen: size of the character buffer at buf
 *
 * fdt_get_path() computes the full path of the node at offset
 * nodeoffset, and records that path in the buffer at buf.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:
 *	0, on success
 *		buf contains the absolute path of the node at
 *		nodeoffset, as a NUL-terminated string.
 *	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_NOSPACE, the path of the given node is longer than (bufsize-1)
 *		characters and will not fit in the given buffer.
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_get_path(const void *fdt, int nodeoffset, char *buf, int buflen);

/**
 * fdt_supernode_atdepth_offset - find a specific ancestor of a node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 * @supernodedepth: depth of the ancestor to find
 * @nodedepth: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_supernode_atdepth_offset() finds an ancestor of the given node
 * at a specific depth from the root (where the root itself has depth
 * 0, its immediate subnodes depth 1 and so forth).  So
 *	fdt_supernode_atdepth_offset(fdt, nodeoffset, 0, NULL);
 * will always return 0, the offset of the root node.  If the node at
 * nodeoffset has depth D, then:
 *	fdt_supernode_atdepth_offset(fdt, nodeoffset, D, NULL);
 * will return nodeoffset itself.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:
 *	structure block offset of the node at node offset's ancestor
 *		of depth supernodedepth (>=0), on success
 *	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_NOTFOUND, supernodedepth was greater than the depth of
 *		nodeoffset
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_supernode_atdepth_offset(const void *fdt, int nodeoffset,
                                 int supernodedepth, int *nodedepth);

/**
 * fdt_node_depth - find the depth of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 *
 * fdt_node_depth() finds the depth of a given node.  The root node
 * has depth 0, its immediate subnodes depth 1 and so forth.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:
 *	depth of the node at nodeoffset (>=0), on success
 *	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_depth(const void *fdt, int nodeoffset);

/**
 * fdt_parent_offset - find the parent of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 *
 * fdt_parent_offset() locates the parent node of a given node (that
 * is, it finds the offset of the node which contains the node at
 * nodeoffset as a subnode).
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset, *twice*.
 *
 * returns:
 *	structure block offset of the parent of the node at nodeoffset
 *		(>=0), on success
 *	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_parent_offset(const void *fdt, int nodeoffset);

/**
 * fdt_node_offset_by_prop_value - find nodes with a given property value
 * @fdt: pointer to the device tree blob
 * @startoffset: only find nodes after this offset
 * @propname: property name to check
 * @propval: property value to search for
 * @proplen: length of the value in propval
 *
 * fdt_node_offset_by_prop_value() returns the offset of the first
 * node after startoffset, which has a property named propname whose
 * value is of length proplen and has value equal to propval; or if
 * startoffset is -1, the very first such node in the tree.
 *
 * To iterate through all nodes matching the criterion, the following
 * idiom can be used:
 *	offset = fdt_node_offset_by_prop_value(fdt, -1, propname,
 *					       propval, proplen);
 *	while (offset != -FDT_ERR_NOTFOUND) {
 *		// other code here
 *		offset = fdt_node_offset_by_prop_value(fdt, offset, propname,
 *						       propval, proplen);
 *	}
 *
 * Note the -1 in the first call to the function, if 0 is used here
 * instead, the function will never locate the root node, even if it
 * matches the criterion.
 *
 * returns:
 *	structure block offset of the located node (>= 0, >startoffset),
 *		 on success
 *	-FDT_ERR_NOTFOUND, no node matching the criterion exists in the
 *		tree after startoffset
 *	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_offset_by_prop_value(const void *fdt, int startoffset,
                                  const char *propname, const void *propval,
                                  int proplen);

/**
 * fdt_node_offset_by_phandle - find the node with a given phandle
 * @fdt: pointer to the device tree blob
 * @phandle: phandle value
 *
 * fdt_node_offset_by_phandle() returns the offset of the node
 * which has the given phandle value.  If there is more than one node
 * in the tree with the given phandle (an invalid tree), results are
 * undefined.
 *
 * returns:
 *	structure block offset of the located node (>= 0), on success
 *	-FDT_ERR_NOTFOUND, no node with that phandle exists
 *	-FDT_ERR_BADPHANDLE, given phandle value was invalid (0 or -1)
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_offset_by_phandle(const void *fdt, uint32_t phandle);

/**
 * fdt_node_check_compatible: check a node's compatible property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @compatible: string to match against
 *
 *
 * fdt_node_check_compatible() returns 0 if the given node contains a
 * 'compatible' property with the given string as one of its elements,
 * it returns non-zero otherwise, or on error.
 *
 * returns:
 *	0, if the node has a 'compatible' property listing the given string
 *	1, if the node has a 'compatible' property, but it does not list
 *		the given string
 *	-FDT_ERR_NOTFOUND, if the given node has no 'compatible' property
 *	-FDT_ERR_BADOFFSET, if nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_check_compatible(const void *fdt, int nodeoffset,
                              const char *compatible);

/**
 * fdt_node_offset_by_compatible - find nodes with a given 'compatible' value
 * @fdt: pointer to the device tree blob
 * @startoffset: only find nodes after this offset
 * @compatible: 'compatible' string to match against
 *
 * fdt_node_offset_by_compatible() returns the offset of the first
 * node after startoffset, which has a 'compatible' property which
 * lists the given compatible string; or if startoffset is -1, the
 * very first such node in the tree.
 *
 * To iterate through all nodes matching the criterion, the following
 * idiom can be used:
 *	offset = fdt_node_offset_by_compatible(fdt, -1, compatible);
 *	while (offset != -FDT_ERR_NOTFOUND) {
 *		// other code here
 *		offset = fdt_node_offset_by_compatible(fdt, offset, compatible);
 *	}
 *
 * Note the -1 in the first call to the function, if 0 is used here
 * instead, the function will never locate the root node, even if it
 * matches the criterion.
 *
 * returns:
 *	structure block offset of the located node (>= 0, >startoffset),
 *		 on success
 *	-FDT_ERR_NOTFOUND, no node matching the criterion exists in the
 *		tree after startoffset
 *	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_offset_by_compatible(const void *fdt, int startoffset,
                                  const char *compatible);

/**
 * fdt_stringlist_contains - check a string list property for a string
 * @strlist: Property containing a list of strings to check
 * @listlen: Length of property
 * @str: String to search for
 *
 * This is a utility function provided for convenience. The list contains
 * one or more strings, each terminated by \0, as is found in a device tree
 * "compatible" property.
 *
 * @return: 1 if the string is found in the list, 0 not found, or invalid list
 */
int fdt_stringlist_contains(const char *strlist, int listlen, const char *str);

/**
 * fdt_stringlist_count - count the number of strings in a string list
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @property: name of the property containing the string list
 * @return:
 *   the number of strings in the given property
 *   -FDT_ERR_BADVALUE if the property value is not NUL-terminated
 *   -FDT_ERR_NOTFOUND if the property does not exist
 */
int fdt_stringlist_count(const void *fdt, int nodeoffset, const char *property);

/**
 * fdt_stringlist_search - find a string in a string list and return its index
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @property: name of the property containing the string list
 * @string: string to look up in the string list
 *
 * Note that it is possible for this function to succeed on property values
 * that are not NUL-terminated. That's because the function will stop after
 * finding the first occurrence of @string. This can for example happen with
 * small-valued cell properties, such as #address-cells, when searching for
 * the empty string.
 *
 * @return:
 *   the index of the string in the list of strings
 *   -FDT_ERR_BADVALUE if the property value is not NUL-terminated
 *   -FDT_ERR_NOTFOUND if the property does not exist or does not contain
 *                     the given string
 */
int fdt_stringlist_search(const void *fdt, int nodeoffset, const char *property,
                          const char *string);

/**
 * fdt_stringlist_get() - obtain the string at a given index in a string list
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @property: name of the property containing the string list
 * @index: index of the string to return
 * @lenp: return location for the string length or an error code on failure
 *
 * Note that this will successfully extract strings from properties with
 * non-NUL-terminated values. For example on small-valued cell properties
 * this function will return the empty string.
 *
 * If non-NULL, the length of the string (on success) or a negative error-code
 * (on failure) will be stored in the integer pointer to by lenp.
 *
 * @return:
 *   A pointer to the string at the given index in the string list or NULL on
 *   failure. On success the length of the string will be stored in the memory
 *   location pointed to by the lenp parameter, if non-NULL. On failure one of
 *   the following negative error codes will be returned in the lenp parameter
 *   (if non-NULL):
 *     -FDT_ERR_BADVALUE if the property value is not NUL-terminated
 *     -FDT_ERR_NOTFOUND if the property does not exist
 */
const char *fdt_stringlist_get(const void *fdt, int nodeoffset,
                               const char *property, int index, int *lenp);

/**********************************************************************/
/* Read-only functions (addressing related)                           */
/**********************************************************************/

/**
 * FDT_MAX_NCELLS - maximum value for #address-cells and #size-cells
 *
 * This is the maximum value for #address-cells, #size-cells and
 * similar properties that will be processed by libfdt.  IEE1275
 * requires that OF implementations handle values up to 4.
 * Implementations may support larger values, but in practice higher
 * values aren't used.
 */
#define FDT_MAX_NCELLS 4

/**
 * fdt_address_cells - retrieve address size for a bus represented in the tree
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node to find the address size for
 *
 * When the node has a valid #address-cells property, returns its value.
 *
 * returns:
 *	0 <= n < FDT_MAX_NCELLS, on success
 *      2, if the node has no #address-cells property
 *      -FDT_ERR_BADNCELLS, if the node has a badly formatted or invalid
 *		#address-cells property
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings
 */
int fdt_address_cells(const void *fdt, int nodeoffset);

/**
 * fdt_size_cells - retrieve address range size for a bus represented in the
 *                  tree
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node to find the address range size for
 *
 * When the node has a valid #size-cells property, returns its value.
 *
 * returns:
 *	0 <= n < FDT_MAX_NCELLS, on success
 *      2, if the node has no #address-cells property
 *      -FDT_ERR_BADNCELLS, if the node has a badly formatted or invalid
 *		#size-cells property
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings
 */
int fdt_size_cells(const void *fdt, int nodeoffset);

/**********************************************************************/
/* Debugging / informational functions                                */
/**********************************************************************/

const char *fdt_strerror(int errval);

#endif /* _SYS_LIBFDT_H_ */
