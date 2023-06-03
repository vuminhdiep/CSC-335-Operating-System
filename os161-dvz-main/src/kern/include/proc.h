/*
 * Copyright (c) 2013
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

#ifndef _PROC_H_
#define _PROC_H_

/*
 * Definition of a process.
 *
 * Note: curproc is defined by <current.h>.
 */

#include <synch.h>
#include <spinlock.h>
#include <limits.h>
#include <filetable.h>

struct addrspace;
struct thread;
struct vnode;

struct pidxcode {
	pid_t ppid;         /* pid of this code's parent*/ //curproc pid == ppid if parent
	pid_t pid;          /* pid that this code is associated with*/
	int exit_code;    /* exit code that this process exited with. */
};

/*
 * Process structure.
 *
 * Note that we only count the number of threads in each process.
 * (And, unless you implement multithreaded user processes, this
 * number will not exceed 1 except in kproc.) If you want to know
 * exactly which threads are in the process, e.g. for debugging, add
 * an array and a sleeplock to protect it. (You can't use a spinlock
 * to protect an array because arrays need to be able to call
 * kmalloc.)
 *
 * You will most likely be adding stuff to this structure, so you may
 * find you need a sleeplock in here for other reasons as well.
 * However, note that p_addrspace must be protected by a spinlock:
 * thread_switch needs to be able to fetch the current address space
 * without sleeping.
 */
struct proc {
	char *p_name;			/* Name of this process */
	struct spinlock p_lock;		/* Lock for this structure */
	unsigned p_numthreads;		/* Number of threads in this process */
	struct lock* ops_lock;
	/* VM */
	struct addrspace *p_addrspace;	/* virtual address space */

	/* VFS */
	struct vnode *p_cwd;		/* current working directory */

	pid_t pid;		/* PID of this process*/
	pid_t ppid;     /* PID of this process's parent*/

	struct p_filetable* p_filetable;

	struct proc** children; //not used in waitpid, should probably delete

	struct cv *waitpid_cv; //cv used for waitpid

	struct lock *waitpid_lock; //dedicated lock for waitpid, not used, can be removed

	
};

struct proctable {
	pid_t next_pid;             /* next pid to be returned to */
	int num_of_procs;         /* holds the number of processes currently running */
	struct proc** proc_array;  /* (Array of Processes) Points to a series of pointers of processes */
	struct lock* proctable_lk; /* lock for proc table operations*/
	struct cv *waitpid_cv; //might delete this because cv should be attached to each proc
	struct pidxcode** active_exit_codes; /* array of exit codes that processes must */

};

struct proctable* proctable_create(void); //create a global process table

void proctable_bootstrap(void); //bootstrap function for the global process table
void proctable_destroy(struct proctable* pt); //destroying a process table

int get_next_pid(void); //getting the next pid

int put_proc_on_proctable(struct proc* p); //put a process on the global proc table

void remove_proc_from_proctable(pid_t pid);//remove a process from the global proc table
struct proc* get_reference_by_pid(pid_t pid); //get a process from the table by pid

//

extern struct proctable *global_ptable; //externally accessible global proc table
extern struct lock *exit_lock;
extern struct lock *fork_lock; //locks for forking and exiting. Unused, can be removed

/* This is the process structure for the kernel and for kernel-only threads. */
extern struct proc *kproc;

/* Call once during system startup to allocate data structures. */
void proc_bootstrap(void);

/* Create a fresh process for use by runprogram(). */
struct proc *proc_create_runprogram(const char *name);

/* Destroy a process. */
void proc_destroy(struct proc *proc);

/* Attach a thread to a process. Must not already have a process. */
int proc_addthread(struct proc *proc, struct thread *t);

/* Detach a thread from its process. */
void proc_remthread(struct thread *t);

/* Fetch the address space of the current process. */
struct addrspace *proc_getas(void);

/* Change the address space of the current process, and return the old one. */
struct addrspace *proc_setas(struct addrspace *);

//handler for the child's part of forking
void child_fork_handler(void * data1, unsigned long data2);

//add a child to a process, unused and can be removed
int add_child_to_proc(struct proc* parent, struct proc* child);

//remove all exit codes associated with this process
void rem_all_exit_codes(pid_t pid);

//post an exit code to the global table
void post_exit_code(int rawexcode, pid_t ppid, pid_t pid);

// remove a child from a parent, unused and can be removed
void remove_child_from_proc(struct proc* remfrom, struct proc* rem);

bool is_proc_exited(struct proctable *ptable, pid_t searchpid); /*Check if a proc is exited*/

bool is_proc_children(struct proc *proc, struct proc *other_proc); /*Check if a proc is a child of other_proc, return true, else false*/

int get_exit_code(struct proctable *ptable, pid_t ppid, pid_t pid); /*Get the corresponding exit code given a ppid and pid, return -1 on fail*/


#endif /* _PROC_H_ */
