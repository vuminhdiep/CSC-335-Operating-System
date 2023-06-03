#include <types.h>
#include <syscall.h>
#include <vnode.h>
#include <synch.h>
#include <copyinout.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <kern/errno.h>
#include <lib.h>
#include <filetable.h>
#include <vfs.h>
#include <spl.h>
#include <current.h>
#include <proc.h>
#include <addrspace.h>
#include <mips/trapframe.h>


//handler to initialize child 
void child_fork_handler(void * data1, unsigned long data2){
    (void) data2;//data 2 unused
    struct trapframe *tfget = (struct trapframe*) data1;//cast a trapframe from data1

    KASSERT(curproc->p_addrspace != NULL);
    as_activate();//activate addr space
    
    struct trapframe tf = *tfget;//trapframe on local stack
    tf.tf_a3 = 0;
    tf.tf_v0 = 0;
    tf.tf_epc = tf.tf_epc + 4;//manipulating registers to allow forward progress
    
    kfree(data1);//free allocated data
    
    mips_usermode(&tf);//warp to user mode with modified trapframe

}

pid_t fork(struct trapframe *tf){

   if (global_ptable->num_of_procs == __PROC_MAX){

        return -1 * ENPROC;//fail if max processes reached
    }
    struct trapframe* copiedtf = kmalloc(sizeof(struct trapframe));
    if (copiedtf == NULL){

        return -1 * ENOMEM;//fail if can't create trapframe
    }
    memcpy(copiedtf, tf, sizeof(struct trapframe));//memcpy trapframe
    
    struct proc* copy = proc_create_runprogram("child_proc");
    if (copy == NULL){//create a process to run, fail if no memory
        kfree(copiedtf);

	return -1 * ENOMEM; 
    }

    
    lock_acquire(curproc->ops_lock); //get current proc operations lock
    copy->pid = get_next_pid();//assign next pid to child
    copy->ppid = curproc->pid;//connect parent and child

    int tst;
    tst = as_copy(curproc->p_addrspace, &copy->p_addrspace);
    
    if (tst){//copy address space, failing if this doesn't work
        kfree(copiedtf);
        proc_destroy(copy);
	    lock_release(curproc->ops_lock);
        return -1 * tst;
    }

    copy_p_filetable(curproc->p_filetable, copy->p_filetable);
    //copy the filetable across

    lock_release(curproc->ops_lock);
    //release current process's lock

    int incaseoferror = 0;
    int toreturn = copy->pid;//set up returns

    put_proc_on_proctable(copy);//put process on table

    //go to user mode and start new thread
    incaseoferror = thread_fork("threadchild", copy, child_fork_handler, (void*) copiedtf, 0);


    if (incaseoferror){
        return -1 * incaseoferror;
    }
    
    return toreturn;



}
