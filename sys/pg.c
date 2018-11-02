#include <pg.h>
#include <proc.h>

int setpgid(pid_t pid, pgid_t pgid) {
	proc_t *p_handle = proc_find(pid);
	proc_lock(p_handle);
	p_handle->p_pgid = pgid;
	proc_unlock(p_handle);
}
