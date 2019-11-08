#include <sys/vmem.h>
#include <sys/queue.h>
#include <sys/sysinit.h>

/* boundary tag */
typedef struct vmem_btag {
	TAILQ_ENTRY(vmem_btag) bt_seglist;
	union {
		LIST_ENTRY(vmem_btag) u_freelist; /* BT_TYPE_FREE */
		LIST_ENTRY(vmem_btag) u_hashlist; /* BT_TYPE_BUSY */
	} bt_u;
	vmem_addr_t bt_start;
	vmem_size_t bt_size;
	int bt_type;
} vmem_btag_t;

typedef TAILQ_HEAD(vmem_seglist, vmem_btag) vmem_seglist_t;
typedef LIST_HEAD(vmem_freelist, vmem_btag) vmem_freelist_t;
typedef LIST_HEAD(vmem_hashlist, vmem_btag) vmem_hashlist_t;

static void vmem_init(void) {

}

SYSINIT_ADD(vmem, vmem_init, NODEPS);
