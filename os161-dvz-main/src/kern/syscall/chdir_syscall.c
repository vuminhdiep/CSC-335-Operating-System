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
#include <kern/errno.h>
#include <uio.h>
#include <kern/iovec.h>
#include <vfs.h>
#include <copyinout.h>
#include <array.h>

int chdir(const_userptr_t pathname)
{
    /*ENODEV	The device prefix of pathname did not exist.
    ENOTDIR	A non-final component of pathname was not a directory.
    ENOTDIR	pathname did not refer to a directory.
    ENOENT	pathname did not exist.
    EIO	A hard I/O error occurred.
    EFAULT	pathname was an invalid pointer.*/

    lock_acquire(fileops_lock);
    int returnval;
    //make the uio so that we can copyinstr 

    //make temporary buffer that is of the pathname 
    char buf[(int)__PATH_MAX];
    

    //ROBUSTNESS
    // if (buf == NULL) {
    //     kprintf("buf failed to allocate");
    //     lock_release(fileops_lock);
    //     return -EIO;
    // }

    size_t GOT = 0;

    //copy in the path to change to
    returnval = copyinstr(pathname, buf, (size_t)__PATH_MAX, &GOT);

    //ROBUSTNESS
    if (returnval != 0) {
        lock_release(fileops_lock);
        return -returnval;
    }

    // 
    // struct vnode** ret_file = NULL;
    // returnval = vfs_lookup(buf, ret_file);
    //copy in the string to the kernel
    returnval = vfs_chdir(buf);

    //ROBUSTNESS
    if (returnval != 0) {
        // kprintf("pathname did not refer to a directory.");
        lock_release(fileops_lock);
        return -returnval;
    }

    // pathname = buf;

    //free the buffer
    // kfree(buf);
    lock_release(fileops_lock);
    return returnval;
}