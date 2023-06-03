#include <types.h>
#include <syscall.h>
#include <kern/wait.h>
#include <vnode.h>
#include <synch.h>
#include <copyinout.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <lib.h>
#include <current.h>
#include <proc.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <wchan.h>
#include <vm.h>

int waitpid(pid_t pid, int *status, int options, pid_t *retval){

    *retval = -1;
    if (options != 0){ //invalid options
        return -EINVAL;
    }
    if (pid < __PID_MIN || pid > __PID_MAX){ //invalid pid
        return -1 * ESRCH;
    }
    lock_acquire(curproc->ops_lock);
    struct proc *child_proc = get_reference_by_pid(pid);
    //lock_acquire(child_proc->waitpid_lock);

    if(child_proc == NULL && !is_proc_exited(global_ptable, pid)){ //non-existent proc in the proctable
        //lock_release(child_proc->waitpid_lock);
        lock_release(curproc->ops_lock);
        return -1 * ESRCH;
    }
    if (is_proc_exited(global_ptable, pid)){
        *retval =pid;
        lock_release(curproc->ops_lock);
        return 0;
    }
    lock_acquire(child_proc->ops_lock);
    if(!child_proc->ppid == curproc->pid || pid ==curproc->pid){ //if proc associated with given pid is not the child of the current process or wait for self
        lock_release(child_proc->ops_lock);
        lock_release(curproc->ops_lock);
        return -1 * ECHILD;
    }
    lock_release(child_proc->ops_lock);

    while(!is_proc_exited(global_ptable, pid)){ //if proc is not exited, keep waiting
        cv_wait(curproc->waitpid_cv, curproc->ops_lock);

    }

    int exit_code = get_exit_code(global_ptable, curproc->pid, pid);

    //if proc already exited, copyoutstr the status value
    if(status != NULL){ //only copyoutstr the exit_code from user space if status is not NULL
        //size_t got;
        int err = copyout((const void*) &exit_code, (userptr_t) status, sizeof(int));
        if(err == EFAULT){ //check for invalid status pointer
            //lock_release(child_proc->waitpid_lock);
            lock_release(curproc->ops_lock);
            return -1 * err;
        }
    }
    
    //It is explicitly allowed for status to be NULL, in which case waitpid operates normally but the status value is not produced.
    //still stall at NULL status

    //lock_release(child_proc->waitpid_lock);
    lock_release(curproc->ops_lock);
    *retval = pid;
    return 0;
    
}








/***
 * use copyoutstr for copy the value pointed by status to the corresponding exit_code
 * if status == NULL, don't copyoutstr, don't care the output
 * check status inalignment by multiple of 4
 * might have to decide the cv in each proc or the globa
 * 
*/

/*
pid_t sys_waitpid(pid_t pid, int *status, int options){
    if (options != 0){ //invalid options
        return EINVAL;
    }
    if (pid < __PID_MIN || pid > __PID_MAX){ //invalid pid
        return ESRCH;
    }

    lock_acquire(global_ptable->proctable_lk);
    struct proc *child_proc = get_reference_by_pid(pid);
       

    pid_t toreturn = -1;
    if (child_proc == NULL){ //non-existent proc in the proctable
        lock_release(global_ptable->proctable_lk);
        return ESRCH;
    }
    if(!is_proc_children(child_proc, curproc)){ //if proc associated with given pid is not the child of the current process
        lock_release(global_ptable->proctable_lk);
        return ECHILD;
    }
    

    if(is_proc_exited(global_ptable, child_proc)){ //if proc already exited, not sure 
    //also need to assign status to exit_code here, maybe put this under the if statement to check for status not NULL to copyoutstr it at the end
        lock_release(global_ptable->proctable_lk);
        return pid;
    }
    if(pid == curproc->pid){ //wait for self
        lock_release(global_ptable->proctable_lk);
        return pid;
    }
    //scan through exit code to signal cv wait
    while(!is_proc_exited(global_ptable, child_proc)){
        cv_wait(global_ptable->waitpid_cv, global_ptable->proctable_lk); //parent keep on waiting for child proc to exit
        
    }


    //check through exitcode if found clear from table
    
    int proc_exit_code = get_exit_code(global_ptable, curproc->pid, pid); //get exit code of the proc associated with the pid

    if (status != NULL && proc_exit_code != -1){ //NOT SURE: only get status with exit code if status not NULL, but how to handle status NULL?
        *status = proc_exit_code; //NOT SURE: assign exit code to status use this or status = &proc_exit_code?
    }
    
    toreturn = pid;
    
    //signal parent to wakeup
    //cv_signal(global_ptable->waitpid_cv, global_ptable->proctable_lk); //don't need this since exit handles already
   
    lock_release(global_ptable->proctable_lk);

     //probably don't need these since exit handle already
     
    // rem_all_exit_codes(pid); 
    // remove_proc_from_proctable(pid); //destroy process
    // proc_destroy(child_proc);

    return toreturn;
}
*/
