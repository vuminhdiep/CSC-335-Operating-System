#include <kern/unistd.h>
#include <types.h>
#include <syscall.h>
#include <filetable.h>
#include <proc.h>
#include <synch.h>
#include <spinlock.h>
#include <thread.h>
#include <current.h>
#include <kern/limits.h>
#include <kern/wait.h>
void _exit(int exitcode)
{

    //lock_acquire(exit_lock);

    lock_acquire(curproc->ops_lock); //get proc operations lock
   
    struct proc* parent_proc = get_reference_by_pid(curproc->ppid);
    // get reference to parent proc


    rem_all_exit_codes(curproc->pid);
    // remove all exit codes associated with this process as the parent
    
    if (parent_proc != NULL){//if parent hasn't exited
	lock_acquire(parent_proc->ops_lock);//lock the parent

            post_exit_code(_MKWAIT_EXIT(exitcode), curproc->ppid, curproc->pid);
            //post exit code to table using _MKWAIT_EXIT
            lock_release(parent_proc->ops_lock);
            //release parent's lock so no deadlock
            cv_signal(parent_proc->waitpid_cv, curproc->ops_lock);
            //wake up potentially waiting parent

    } 
    remove_proc_from_proctable(curproc->pid);
    //remove this process from the process table.

    lock_release(curproc->ops_lock);
    //release this proc's lock and let thread_exit do the rest.

    thread_exit();
    // =====================================
    // NOTE:
    // 1: 
    // address space deallocation is handled automatically by proc_destroy.
    // 2:
    //Thread_exit calls proc_remthread(), which calls proc_destroy to deallocate proc and addr space.
    // exit does deallocate the process, even if this isn't immediately obvious.


}
