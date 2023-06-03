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
#include <uio.h>

//zeros out buffer, used in debug, can probably be removed.
static void init_write_buffer(char* buffer, size_t len){
    for (size_t i = 0; i < len; i++){
        buffer[i] = 0;
    }
}

ssize_t write(int fd, const void *buf, size_t buflen){
    if (buf == NULL){ //fail if buffer is null
	return -1 * EFAULT;
    }

    //lock the current proc
    lock_acquire(curproc->ops_lock);
    struct p_filetable* local_table = curproc->p_filetable;

    struct file* f = p_filetable_lookup(local_table, fd);
    //get file and look up in file table

    if (f == NULL){
        lock_release(curproc->ops_lock);
        return -1 * EBADF;
    }//fail if file not found

    lock_acquire(f->file_lock);//lock file
    if (!f->can_write){
        lock_release(f->file_lock);
        lock_release(curproc->ops_lock);
        return -1 * EBADF;
    }//fail if file can't write


    ssize_t ret_val = 0;

    if ( f->std_file_status == STDOUT_FILENO || f->std_file_status == STDERR_FILENO){
        
        if (local_table->stderr_open || local_table->stdin_open){
            lock_release(curproc->ops_lock);
        //if this is a standard out/error and these channels are open, continue...
        char* write_buffer = kmalloc(sizeof(char) * buflen);
        if (write_buffer == NULL){
            lock_release(f->file_lock);
            return -1 * ENOMEM;
        }
        size_t got ;
        int tst;
        tst = copyinstr((const_userptr_t)buf, write_buffer, buflen, &got);
        if (tst == EFAULT){//copyin string to be printed
            kfree(write_buffer);
            lock_release(f->file_lock);
            return -1 * tst;
        }
        //print the string in a loop
        for(size_t i = 0; i <= buflen; i++){
            if (32 <= write_buffer[i] || write_buffer[i] == 10){
		        putch(write_buffer[i]);}
	    } 
        lock_release(f->file_lock);
        kfree(write_buffer);
        int retval = strlen(write_buffer) + 1;//calculate values
        
        return retval;//return
        }else{ //...else fail if these channels are closed.
            lock_release(f->file_lock);
            lock_release(curproc->ops_lock);

            return -1 * EBADF; 
        }

    }
    

    lock_release(curproc->ops_lock); //release process lock, file is being modified now

    char* write_buffer = kmalloc(sizeof(char) * buflen);
        if (write_buffer == NULL){
            lock_release(f->file_lock);
            return -1 * ENOMEM;
        }//create write buffer and fail if no mem
    init_write_buffer(write_buffer, buflen); //initialize write buffer, can be removed.

    int tst;
    tst = copyin((userptr_t) buf, write_buffer, buflen);
    //copyin the data
	
    if (tst == EFAULT){
	kfree(write_buffer);
	lock_release(f->file_lock);
	return -1 * tst;//fail if copy failed
    }
    
    struct iovec iov;
    struct uio myuio;
    uio_kinit(&iov, &myuio, write_buffer, buflen, *f->seek_pos, UIO_WRITE);
    //initialize uio
    
    ret_val = VOP_WRITE(f->file_vnode, &myuio);//write contents to file
    if (ret_val){
        kfree(write_buffer);
        lock_release(f->file_lock);

        return -1 * ret_val; //fail if faulted on write

    }
    
    ret_val =  buflen - myuio.uio_resid;
    *f->seek_pos = myuio.uio_offset; //calculate returns and update seek position

    kfree(write_buffer);
    lock_release(f->file_lock);//free buffer and unlock file.
    return ret_val;
}
