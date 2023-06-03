#include <types.h>
#include <kern/unistd.h>
#include <syscall.h>
#include <proc.h>
#include <current.h>


int getpid(pid_t *retval){//gets and returns pid, tested by waitpid tests.
    lock_acquire(curproc->ops_lock);
    *retval = curproc->pid;
    lock_release(curproc->ops_lock);
    return 0;
}

