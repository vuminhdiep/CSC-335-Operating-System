#include <types.h>
#include <kern/limits.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <syscall.h>
#include <filetable.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>
#include <vnode.h>
#include <synch.h>
#include <lib.h>
#include <vfs.h>


int dup2(int oldfd, int newfd){
    

    if (oldfd < 0 || newfd < 0 || newfd >= __OPEN_MAX){ //invalid fd
        return -EBADF;
    }
    if (oldfd == newfd){ //file dup itself
        return newfd;
    }
    lock_acquire(curproc->ops_lock);//lock the process
    
    int toreturn;
    struct p_filetable *p_ft = curproc->p_filetable;
    struct file *old_entry;
    struct file *new_entry;//get local table and set up containers

    //PCB has mu
   
    old_entry = p_filetable_lookup(p_ft, oldfd); //get the file with oldfd
    new_entry = p_filetable_lookup(p_ft, newfd); //get the file with newfd

    if(old_entry == NULL){ //if file not exist throw an error
        lock_release(curproc->ops_lock);
        return -EBADF;
    }


    //Check if newfd is opened, then close the file in p_ft
    if(new_entry != NULL){
        close_file_on_ptable(p_ft, newfd);
    }
    
    new_entry = dup_file(p_ft, oldfd, newfd);

    //put new_entry to per proc file table
    
    toreturn = put_file_on_ptable(p_ft, new_entry);
    if(toreturn == -1){ //reach maximum file table limit

        lock_release(curproc->ops_lock);
        return -EMFILE;
    }
    else{
        toreturn = newfd;
    }
    
    lock_release(curproc->ops_lock);

    return toreturn; 
}
