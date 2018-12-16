#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "utest.h"

static void reset(void) {
	setpgid(0, 0);
}

int test_pgid_args(void){
	reset();
	
	assert(getpgid(0) == getpgid(getpid()));

	return 0;
}

int test_pgid_misbehave(void){
	reset();
	// should not be able to set pgid to 1
	assert(-1 == setpgid(1, 0));

	// check if nothing changed
	assert(getpgid(0) == getpid());

	return 0;
}

int test_pgid_inheritance(void){
	reset();
	pid_t prev_pgid = getpgid(0);
	pid_t pid = fork();
	if(pid == 0) {
		// child
		// should inherit pgid from parent
		assert(getpgid(0) == prev_pgid);
		// child should be unable to change pid of parent
		assert(-1 == setpgid(prev_pgid, getpgid(0)));

		assert(!setpgid(0, 0)); // set pgid to process id
		assert(getpgid(0) == getpid());
		assert(getpgid(getpid()) == getpgid(0));
		// assert(0); TODO: does it report failure when triggered in child process??
		exit(0);
	}
	// waitpid(pid);
	wait(NULL);

	// ensure pgid was not changed by child
	assert(getpgid(0) == prev_pgid);

	return 0;
}

int test_pgid_change_by_parent(void){
	reset();
	pid_t prev_pgid = getpgid(0);

	pid_t pid = fork();
	if(pid == 0){
		// child
		// wait until parent changes its pgid
		while(getpgid(0) == prev_pgid)
		{}

		assert(getpgid(0) == getpid());
		exit(0);
	}
	// change pgid of child, so 
	assert(!setpgid(pid, pid));	
	
	// waitpid(pid);
	wait(NULL);

	return 0;
}


