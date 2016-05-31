#include <tlb.h>
#include <vm_map.h>
#include <malloc.h>
#include <libkern.h>
#include <mips.h>
#include <pmap.h>
#include <tree.h>
    
vm_map_t *cur_vm_map;

static inline int vm_page_cmp(vm_page_t *a, vm_page_t *b)
{
    if(a->vm_offset < b->vm_offset)
        return -1;
    return a->vm_offset-b->vm_offset;
}

static inline int vm_map_entry_cmp(vm_map_entry_t *a, vm_map_entry_t *b)
{
    if(a->start < b->start)
        return -1;
    return a->start-b->start;
}

RB_PROTOTYPE_STATIC(vm_object_tree, vm_page, obj_tree, vm_page_cmp);
RB_GENERATE(vm_object_tree, vm_page, obj_tree, vm_page_cmp);

SPLAY_PROTOTYPE(vm_map_tree, vm_map_entry, map_tree, vm_map_entry_cmp);
SPLAY_GENERATE(vm_map_tree, vm_map_entry, map_tree, vm_map_entry_cmp);

static MALLOC_DEFINE(mpool, "inital vm_map memory pool");

static vm_map_entry_t *allocate_entry()
{
    vm_map_entry_t *entry = kmalloc(mpool, sizeof(vm_map_entry_t), 0);
    if(!entry)
        panic("[vm_map] no memory in memory pool");
    return entry;
}

static vm_object_t *allocate_object()
{
    vm_object_t *obj = kmalloc(mpool, sizeof(vm_object_t), 0);
    if(!obj)
        panic("[vm_map] no memory in memory pool");
    TAILQ_INIT(&obj->list);
    RB_INIT(&obj->tree);
    obj->refcount = 0;
    return obj;
}

static void insert_entry(vm_map_t *vm_map, vm_map_entry_t *entry)
{
    if(SPLAY_INSERT(vm_map_tree, &vm_map->tree, entry)) /* insert failed */
        return;
    vm_map_entry_t *next = SPLAY_NEXT(vm_map_tree, &vm_map->tree, entry);
    if(next)
    {
        TAILQ_INSERT_BEFORE(next, entry, map_list);
    }
    else
    {
        TAILQ_INSERT_TAIL(&vm_map->list, entry, map_list);
    }
    vm_map->nentries++;
}

void vm_map_init()
{
    vm_page_t *pg = vm_phys_alloc(2);
    kmalloc_init(mpool);
    kmalloc_add_arena(mpool, pg->virt_addr, PG_SIZE(pg));
    cur_vm_map = NULL;
}

vm_map_t* vm_map_new(vm_map_type_t t, asid_t asid)
{

    vm_map_t *vm_map = kmalloc(mpool, sizeof(vm_map_t), 0);
    TAILQ_INIT(&vm_map->list);
    SPLAY_INIT(&vm_map->tree);
    vm_map->nentries = 0;
    vm_map->asid = asid;

    /* Add page table entry for bootstrap */
    vm_object_t *pt_object = allocate_object();

    switch(t)
    {
        case KERNEL_VM_MAP:
        {
            vm_map->start = KERNEL_VM_MAP_START;
            vm_map->end = KERNEL_VM_MAP_END;
            vm_map_entry_t *pt_entry =  allocate_entry();
            pt_entry->start = PT_BASE;
            pt_entry->end   = PT_BASE+PT_SIZE;
            pt_entry->object = allocate_object();
            pt_entry->object->refcount++;
            pt_entry->object->size = pt_entry->end-pt_entry->start;
            insert_entry(vm_map, pt_entry);
            break;
        }
        default:
            panic("Type %d unimplemented\n", t);
    }

    pmap_init(&vm_map->pmap, pt_object);
    return vm_map;
}

void vm_object_delete(vm_object_t *obj)
{
    while(!TAILQ_EMPTY(&obj->list) )
    {
        vm_page_t *pg = TAILQ_FIRST(&obj->list);
        TAILQ_REMOVE(&obj->list, pg, obj_list);
        vm_phys_free(pg);
    }
    kfree(mpool, obj);
}

void vm_map_remove_entry(vm_map_t *vm_map, vm_map_entry_t *entry)
{
    vm_map->nentries--;
    if(entry->object && entry->object->refcount <= 1)
        vm_object_delete(entry->object);
    TAILQ_REMOVE(&vm_map->list, entry, map_list);
    kfree(mpool, entry);
}

void vm_map_delete(vm_map_t *vm_map)
{
    while(vm_map->nentries > 0)
    {
        vm_map_remove_entry(vm_map, TAILQ_FIRST(&vm_map->list));
    }
    kfree(mpool, vm_map);
}

static void vm_object_allocate_range(vm_object_t *obj, vm_addr_t start_offset, vm_addr_t end_offset)
{
    vm_addr_t cur_offset = start_offset;
    size_t pg_size = end_offset-cur_offset;
    for(int i = VM_NFREEORDER-1; i >= 0 && pg_size > 0; i--)
    {
        while( (1 << i)*PAGESIZE <= pg_size && pg_size > 0)
        {
            vm_page_t *pg = vm_phys_alloc(i);
            pg->vm_offset = cur_offset;
            vm_object_insert_page(obj, pg);
            cur_offset += (1 << i)*PAGESIZE;
            pg_size = end_offset-cur_offset;
        }
    }
}

void vm_map_entry_attach_pages(vm_map_entry_t *entry, vm_addr_t start, vm_addr_t end)
{
    if(!entry->object)
    {
        entry->object = allocate_object();
        entry->object->size = entry->end-entry->start;
        entry->object->refcount++;
    }
    vm_object_allocate_range(entry->object, start-entry->start, end-entry->start);
}

vm_map_entry_t* vm_map_add_entry(vm_map_t* vm_map, uint32_t flags, size_t length, size_t alignment)
{
    length = align(length,PAGESIZE);

    vm_map_entry_t* entry = allocate_entry();
    entry->flags = flags;

    entry->start = vm_map->start;
    entry->start = align(entry->start, alignment);
    entry->end = entry->start+length;

    vm_map_entry_t *etr_it;
    /* Find place on sorted list, first fit policy */
    TAILQ_FOREACH(etr_it, &vm_map->list, map_list)
    {
        entry->start = align(etr_it->end, alignment);
        entry->end = entry->start+length;
        if(TAILQ_NEXT(etr_it, map_list))
        {
            if(entry->end <= TAILQ_NEXT(etr_it, map_list)->start)
                break;
        }
        else
            break;
    }

    insert_entry(vm_map, entry);
    assert(entry->end-entry->start == length);
    return entry;
}

vm_map_entry_t *vm_map_find_entry(vm_map_t *vm_map, vm_addr_t vaddr)
{
    vm_map_entry_t *etr_it;
    TAILQ_FOREACH(etr_it, &vm_map->list, map_list)
    {
        if(etr_it->start <= vaddr && vaddr < etr_it->end)
            return etr_it;
    }
    return NULL;
}

void vm_map_print(vm_map_t *vm_map)
{
    vm_map_entry_t *it;
    TAILQ_FOREACH(it, &vm_map->list, map_list)
    {
        kprintf("[vm_map] entry (%p, %p)\n", it->start, it->end);
    }
}

void vm_object_insert_page(vm_object_t *obj, vm_page_t *page)
{
    if(RB_INSERT(vm_object_tree, &obj->tree, page))
        return;
    vm_page_t *next = RB_NEXT(vm_object_tree, &obj->tree, page);
    if(next)
    {
        TAILQ_INSERT_BEFORE(next, page, obj_list);
    }
    else
    {
        TAILQ_INSERT_TAIL(&obj->list, page, obj_list);
    }
}

void vm_object_print(vm_object_t *obj)
{
    vm_page_t *it;
    RB_FOREACH(it, vm_object_tree, &obj->tree)
    {
        kprintf("[vm_object] offset: %p, order: %d \n", it->vm_offset, it->order);
    }
}

void set_vm_map(vm_map_t* vm_map)
{
    if(!vm_map) return;
    mips32_set_c0(C0_ENTRYHI, vm_map->asid);
    cur_vm_map = vm_map;

    tlb_set_page_table(&vm_map->pmap);
}
