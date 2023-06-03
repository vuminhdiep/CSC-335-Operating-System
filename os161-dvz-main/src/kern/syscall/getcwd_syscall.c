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

/*lingering questions:
   - Allocates buf?
   - Queries vfs
   - Looks up cwd in PCB?
   - Copies out return string?
   - Deallocates buf? 
   - Robust on vfs fail
   - Robust on bad buff ptr
   - Robust on allocation 
*/

int __getcwd(const_userptr_t buf, size_t buflen)
{
    if (buf == NULL){
        return -EFAULT;
    }
    /* ENOENT	A component of the pathname no longer exists.
        EIO	A hard I/O error occurred.
        EFAULT	buf points to an invalid address.*/
    int returnval = 0;

    // lock both the file and the current proc
    lock_acquire(curproc->ops_lock);

    char tempbuf[(int)__PATH_MAX];
    size_t GOT = 0;
    returnval = copyinstr(buf, tempbuf, (size_t)__PATH_MAX, &GOT);

    //ROBUSTNESS
    if (returnval != 0) {
        lock_release(curproc->ops_lock);
        return -returnval;
    }

    struct iovec iov;
    struct uio myuio;  
    uio_kinit(&iov, &myuio, tempbuf, buflen, 0, UIO_READ);
    // EIO check after this? or this?
    returnval = vfs_getcwd(&myuio);

    // check what vfs_getcwd returns
    // if it returns a nonzero value, it means that it fails.
    if (returnval != 0) {
        lock_release(curproc->ops_lock);
        return -returnval;
    } 

    lock_release(curproc->ops_lock);
    return (int)buflen - (int)myuio.uio_resid;
}