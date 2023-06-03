#include <types.h>
#include <syscall.h>
#include <vnode.h>
#include <synch.h>
#include <copyinout.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <lib.h>
#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
int open(const char *filename, int flags){
    // basic checks for quickest fail conditions
    if (filename == NULL){
        return -1 * EFAULT;
    }
    if (global_ftable->size == __OPEN_MAX){
        return -1 * ENFILE;
    }
    // locking process
    lock_acquire(curproc->ops_lock);
    //getting process file table
    struct p_filetable* local_table = curproc->p_filetable;
    if (local_table->size == __OPEN_MAX){
        lock_release(curproc->ops_lock); //fail if table is full
        return -1 * EMFILE;
    }
    char* fpath = kmalloc((size_t)__PATH_MAX);
    if (fpath == NULL){ //allocate path buffer and fail if no mem
        lock_release(curproc->ops_lock);
        return -1 * ENOMEM;
    }
    size_t* got = 0;
    int tst;
    tst = copyinstr( (const_userptr_t) filename, fpath, (size_t) __PATH_MAX, got);
    if (tst == EFAULT){
        kfree(fpath);//copyin the path string and fail if pointer is wrong
        lock_release(curproc->ops_lock);
        return -1 * EFAULT;
    }
    int toreturn = -1;

    struct file* fp;
    fp = create_file(fpath);//create a file structure, fail if not enough mem
    if (fp == NULL){
        kfree(fpath);
        lock_release(curproc->ops_lock);
        return -1 * ENOMEM;
    }
    struct vnode* node;

    int write_discriminator = flags & O_ACCMODE; // & the flags woith these and set
                                                // read/write flags according to the results

    if (write_discriminator == O_RDONLY){
        fp->can_read = 1;
        fp->can_write = 0;
    } else if (write_discriminator == O_WRONLY){
        fp->can_read = 0;
        fp->can_write = 1;
    } else if (write_discriminator == O_RDWR){
        fp->can_read = 1;
        fp->can_write = 1;
    } else{
        return -1 * EINVAL; //fail if the flags don't make sense
    }


    //int tst;
    tst = vfs_open(fpath, flags, 0, &node); 
    if (tst){ //actually open the file in vfs, fail if this doesn't work
        kfree(fpath);
        lock_release(curproc->ops_lock);
        return -1 * tst;// this function will return most necessary error codes 
    }
   
    fp->file_vnode = node;
    fp->global_fd = get_next_fd();
    fp->local_fd = fp->global_fd; //setting up file descriptors 

    toreturn = fp->local_fd;
    put_file_on_gtable(global_ftable, fpath, fp);
    put_file_on_ptable(local_table, fp); //put file on tables
    kfree(fpath);//free path buffer

    lock_release(curproc->ops_lock);
    return toreturn;




}
