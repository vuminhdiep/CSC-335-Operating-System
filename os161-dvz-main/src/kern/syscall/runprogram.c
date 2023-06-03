/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than runprogram() does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <copyinout.h>
#include <limits.h>
#include <thread.h>
#include <setjmp.h>
/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
// edit this so that it takes the args
int
runprogram(char *progname, char** args, unsigned long nargs)
{
	// args = args;
	// kprintf("programname: %s|\n", progname);
	// kprintf("sizeof args: %d\n", sizeof(args));
	// kprintf("sizzeof one thing of args: %d\n", sizeof(args[0]));
	// kprintf("thing: %s\n", args[0]);
	// int i  = 0;
	// while (args[i] != NULL){
	// 	kprintf("%s\n", args[i]);
	// 	i++;
	// }
	// kprintf("num of args: %d\n", i);
	// int argc = sizeof(args)/sizeof(args[0]);
	// int argc = i;
	struct addrspace *as;
	struct vnode *v;
	// vaddr_t is also of userpointer 
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new process. */
	KASSERT(proc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(as);
	as_activate();
	// as_prepare_load(as);  // call before loading executable into addrspace

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	// get the adjusted stack pointer of where the arguments are in userspace
	size_t got = 0;
	vaddr_t adjusted_stk_ptr = stackptr;
	vaddr_t adj_stk_ptr_history[nargs];
	// kprintf("before the copyoutstr\n");
	// kprintf("nargs: %lu\n", nargs);
	for (int i = 1; i < (int)nargs; i++)//edited
	{
		// kprintf("enetering for loop\n");  // print statments for debugging
		// kprintf("arg: %s\n", args[i]);  
		// adjust the stack pointer by the length of the string argument  + 1 for the NULL termination
		result = copyoutstr(args[i], (userptr_t)adjusted_stk_ptr - (strlen(args[i]) + 1), (size_t)strlen(args[i])+1, &got);
		// kprintf("exited out of copyoutstr\n");
		if (result){
			return result;
		}
		/* shift the stack pointer for each arugment added
		 each char is 1 byte long and the "got" variable contains 
		 the length of the string + the NULL termination
		 so we derefernece the value out of got and then add it to
		 the adjusted stack pointer*/ 
		// kprintf("before the math where adjstkptr gets subtracted\n");

		// make sure that got is always a multiple of 4 before adjusting the stack pointer
		// otherwise the arguments will be shifted slightly
		adjusted_stk_ptr -= got;
		if (got%4 != 0) {
			adjusted_stk_ptr -= (4- (got%4));
		}

		// store the pointers to the arguments in here to be retrieved later when we take out the array
		adj_stk_ptr_history[i] = adjusted_stk_ptr;
		// kprintf("exiting for loop\n");
	}
	// kprintf("after copyoutstr\n");
	//check if copyout failed
	// if (result)
	// 	return result;

	// for (int i = 0; i < (int)nargs; i++)
	// {
	// 	result = copyout((const void*)args[i], (userptr_t)argv[i], 4);
	// 	if (result)
	// 		return result;
	// } 
	// kprintf("done with copy out from for loop");

	// push array to the stack
	// struct userptr_t* argv[nargs];  // array that points to pointers of the argument
	// result = copyout((const void*)argv, (userptr_t)adjusted_stk_ptr - sizeof(argv), sizeof(argv));
	// if (result){
	// 	return result;
	// }
	// // shift the pointer accordingly
	// adjusted_stk_ptr -= sizeof(argv);

	// then place all the pointers to the args into the array which is on the stack
	for (int i = 1; i < (int)nargs; i++) //edited
	{
		result = copyout((const void*)adj_stk_ptr_history[i], (userptr_t)adjusted_stk_ptr - (4*i), 4);
		if (result){
			return result;
		}
	}
	// shift the pointer down for all the arguments we pass through
	//adjusted_stk_ptr -= (4 * ((int)nargs - 1));//edited
	// kprintf("before final copyout");
	// result = copyout((const void*)argv, (userptr_t)adjusted_stk_ptr - (sizeof(argv)), (size_t)sizeof(argv));
	// if (result){
	// 	return result;
	// 	}
	// adjusted_stk_ptr -= sizeof(argv);

	// kprintf("finish final copyout");


	// kprintf("before enter new proc\n");
	/* Warp to user mode. */

	// kprintf("modulo 4 of args: %d\n", sizeof(args)%4);
	// kprintf("modulo 4 of args[0]: %d\n", sizeof(args[0])%4);
	// kprintf("about the enter new process");
	if (nargs > 1){
	enter_new_process(nargs - 1 /*argc*/, (userptr_t)adjusted_stk_ptr /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);
	}
	else {
		enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);
	}
	
	//}
	//else{
	//	enter_new_process(argc /*argc*/, NULL /*userspace addr of argv*/,
	//		  NULL /*userspace addr of environment*/,
	//		  stackptr, entrypoint);
	//}

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

