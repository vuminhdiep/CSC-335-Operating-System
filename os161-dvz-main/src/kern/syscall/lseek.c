#include <types.h>
#include <proc.h>
#include <filetable.h>
#include <current.h>
#include <kern/unistd.h>
#include <syscall.h>
#include <synch.h>
#include <kern/limits.h>
#include <lib.h>
#include <vfs.h>
#include <kern/errno.h>
#include <vnode.h>
#include <kern/seek.h>
#include <kern/unistd.h>
#include <kern/stat.h>
#include <kern/fcntl.h>


int lseek(int fd, off_t pos, int whence, off_t* retval){

    if(fd < 0 || fd >= __OPEN_MAX){ //if fd is invalid
        return -EBADF;
    }


    lock_acquire(curproc->ops_lock);
    struct p_filetable* p_ft = curproc->p_filetable;

    
    struct file *cur_file = p_filetable_lookup(p_ft, fd);
    lock_release(curproc->ops_lock);

    if(cur_file == NULL){ //non existent file could not be seeked

        return -EBADF;
    }
    lock_acquire(cur_file->file_lock);

    if(cur_file->std_file_status != -1){ //cannot seek stdin, stdout, stderr files

        lock_release(cur_file->file_lock);
        return -ESPIPE;
    }


    if(!VOP_ISSEEKABLE(cur_file->file_vnode)){ //file is not seekable

        lock_release(cur_file->file_lock);
        return -ESPIPE;
    }
    off_t new_pos = 0;
    off_t eof = 0;
    struct stat *file_stat;
    file_stat = kmalloc(sizeof(struct stat));
    int failure;
    failure = VOP_STAT(cur_file->file_vnode, file_stat); //get file size in bytes
    if (failure){
        lock_release(cur_file->file_lock);
        return -failure;
    }
    eof = file_stat->st_size;
    kfree(file_stat);


    switch (whence)
    {
    case SEEK_SET:
        new_pos = pos;
        break;
    case SEEK_CUR:
        new_pos = *cur_file->seek_pos + pos;
        break;
    case SEEK_END:
        new_pos = eof + pos;
        break;
    default: //invalid whence

       lock_release(cur_file->file_lock);
        return -EINVAL;

    }

    if(new_pos < 0){ //resulting seek is negative

        lock_release(cur_file->file_lock);
        return -EINVAL;
    }
    *cur_file->seek_pos = new_pos;
    *retval = new_pos;
    

    lock_release(cur_file->file_lock);

    return 0;

}
