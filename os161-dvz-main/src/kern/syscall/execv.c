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
#include <test.h>

/*
The pathname of the program to run is passed as program. 
The args argument is an array of 0-terminated strings. 
The array itself should be terminated by a NULL pointer.

The argument strings should be copied into the new process 
as the new process's argv[] array. In the new process, 
argv[argc] must be NULL.

By convention, argv[0] in new processes contains the name 
that was used to invoke the program. This is not necessarily 
the same as program, and furthermore is only a convention 
and should not be enforced by the kernel.

The process file table and current working directory are 
not modified by execv.

The process file table and current working directory are not 
modified by execv.

The maximum total size of the argv (and environment, if any) 
data is given by the system constant ARG_MAX.

ERROR CODES:
ENODEV	The device prefix of program did not exist.
ENOTDIR	A non-final component of program was not a directory.
ENOENT	program did not exist.
EISDIR	program is a directory.
ENOEXEC	program is not in a recognizable executable file format, was for the wrong platform, or contained invalid fields.
ENOMEM	Insufficient virtual memory is available.
E2BIG	The total size of the argument strings exceeeds ARG_MAX.
EIO	A hard I/O error occurred.
EFAULT	One of the arguments is an invalid pointer.
*/

int execv(/*originally char* */ const_userptr_t program, /*char ** */ userptr_t args)
{
    if (program == NULL || args == NULL){
        return -EFAULT;
    }

    
    kprintf("inside execv\n");
    /*
    + execv                     / 10
   - Copies in program string
   - Copies in complex args
   - Copies out (new user space which is the address space)
   - Does basic runprogram exec
   - Destroys old per proc file table 
   - Create new per proc file table 
   - Destroys addrspace 
   - Robust on allocate
   - Robust on bad ptrs 
   - Robust on args limit
   - Robust on arg limit

    */

    // (void)program;
    // (void)args;

    // destroy the current proc so that it gets replaced with the new one
    
    // start by locking down the proc
    // spinlock_acquire(&curproc->p_lock);
    
    // //remove its reference from the global proc table
    // remove_proc_from_proctable(curproc->pid);

    // spinlock_release(&curproc->p_lock);
    
    // //destory the proc
    // proc_destroy(curproc);

    // kprintf("ready to begin executing the other thing");
    // start executing the new program

    int returnval = 0;

    // copies in program string
    // allocate a buffer and GOT to copy in the string of the program name
    char buf[(size_t)__NAME_MAX];
    size_t GOT = 0;
    returnval = copyinstr(program, buf, (size_t)__NAME_MAX, &GOT);
    
    //ROBUSTNESS
    if (returnval != 0){
        return -returnval;
    }
    kprintf("COPIED 1\n");

    /* divide by the size of the char** args by the size of 
    an element in the array in order to calculate the number
    of elements inside args. use that value for argc*/
    // int argc = sizeof(args)/sizeof(args[0]);

    // init an array for the arguments coming with the size = to the # of args
    char* argv[__ARG_MAX];
    // (void)argv;

    // copyin the pointers to function arguments so that they're now in kernel space
    // the arg array pointer is gonna be of 4 bytes in length
    returnval = copyin((const_userptr_t) args, argv, 4);
    //ROBUSTNESS
    if (returnval != 0)
        return -returnval;

    /*calculate the number of args by iterating through the array until I find NULL*/
    int argc  = 0;
	while (argv[argc] != NULL){ 
		//kprintf("%s\n", argv[argc]);
		argc++;
	}
    //kprintf("argc: %d\n", argc);
    
    // now check if the number of args exceeds our max amount of allowed args
    if ((int)__ARG_MAX <= argc) 
        return -E2BIG;
    // init an array for char** array of arguments for the stack use in kernel
    char* args4stack[argc];

    size_t argGOT = 0;
    /*
        iterate through our array of arguments now in kernel space and copy 
        each individual argument string into our char* array 
    */
    for (int i = 0; i < argc; i++) {//edited
        //kprintf("copinstr arg %s\n", (char*)argv[i]);
        returnval = copyinstr((const_userptr_t) argv[i], args4stack[i], __ARG_MAX, &argGOT);

        //ROBUSTNESS
        if (returnval != 0) 
            return -returnval;
    }

    // copyout((const void*) args4stack, );
    kprintf("TRIED TO GO HERE EVEN THOUGH THAT'S BAD");
    // run program does not return on error. Do I still need to check for errors here then?
    returnval = runprogram(buf, args4stack, (unsigned long)argc);
    if (returnval != 0)
        return -returnval;
    return returnval;
}

/*
The execve call is the same as execv except that a NULL-terminated 
list of environment strings (of the form var=value) is also passed 
through. In Unix, execv is a small wrapper for execve that 
supplies the current process environment. In OS/161, execv is the 
primary exec call and execve is not supported or needed unless 
you put in extra work to implement it.
*/

