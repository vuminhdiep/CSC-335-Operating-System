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
#include <current.h>
#include <proc.h>

int close(int fd){
    
    lock_acquire(curproc->ops_lock);//lock the process

    struct file* toclose = p_filetable_lookup(curproc->p_filetable, fd);
    //lookup file in table

    if (toclose == NULL){
	lock_release(curproc->ops_lock);
        return -1 * EBADF;//fail if no file found
    }
    
    //if the file found isn't a dupe and is a standard file, close the 
    //standard file on this table. This is necessary to check because of
    //how our file system works.
    if (!toclose->is_dupe && toclose->std_file_status == STDIN_FILENO){
        curproc->p_filetable->stdin_open = false;
	lock_release(curproc->ops_lock);
        return 0;
    }
    if (!toclose->is_dupe && toclose->std_file_status == STDOUT_FILENO){
        curproc->p_filetable->stdout_open = false;
	lock_release(curproc->ops_lock);
        return 0;
    }
    if (!toclose->is_dupe && toclose->std_file_status == STDERR_FILENO){
        curproc->p_filetable->stderr_open = false;

        lock_release(curproc->ops_lock);
        return 0;
    }

    //if not standard close on proc table.

    close_file_on_ptable(curproc->p_filetable, fd); //
    //=============== NOTE ==================

    //close_file_on_ptable automatically updates global table
    //close_file_on_ptable also works correctly with dupes automatically

    //==========================================

    
    lock_release(curproc->ops_lock);
	
    return 0;


}
