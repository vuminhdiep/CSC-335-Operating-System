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

/*
 * Process support.
 *
 * There is (intentionally) not much here; you will need to add stuff
 * and maybe change around what's already present.
 *
 * p_lock is intended to be held when manipulating the pointers in the
 * proc structure, not while doing any significant work with the
 * things they point to. Rearrange this (and/or change it to be a
 * regular lock) as needed.
 *
 * Unless you're implementing multithreaded user processes, the only
 * process that will have more than one thread is the kernel process.
 */

#include <types.h>
#include <spl.h>
#include <proc.h>
#include <synch.h>
#include <current.h>
#include <addrspace.h>
#include <filetable.h>
#include <vnode.h>
#include <kern/limits.h>
#include <filetable.h>
/*
 * The process for the kernel; this holds all the kernel-only threads.
 */
struct proc *kproc;

/* global process table*/
struct proctable *global_ptable;
struct lock *exit_lock;
struct lock *fork_lock;

/*
 * Create a proc structure.
 */
static
struct proc *
proc_create(const char *name)
{
	struct proc *proc;

	proc = kmalloc(sizeof(*proc));
	if (proc == NULL) {
		return NULL;
	}
	proc->p_name = kstrdup(name);
	if (proc->p_name == NULL) {
		kfree(proc);
		return NULL;
	}

	proc->p_numthreads = 0;
	spinlock_init(&proc->p_lock);
	
	/* VM fields */
	proc->p_addrspace = NULL;
	proc->pid = 0;
	proc->ppid = -1;

	/* VFS fields */
	proc->p_cwd = NULL;

	proc->p_filetable = p_filetable_create();
	if (proc->p_filetable == NULL){
		spinlock_cleanup(&proc->p_lock);
		kfree(proc->p_name);
		kfree(proc);
		return NULL;
	}

	proc->children = kmalloc(sizeof(struct proc*) * __PROC_MAX); /*replace with OPEN_MAX later*/
	if (proc->children == NULL){
		p_filetable_destroy(proc->p_filetable);
		spinlock_cleanup(&proc->p_lock);
		kfree(proc->p_name);
		kfree(proc);
		return NULL;
	}
	for (int i = 0; i < __PROC_MAX; i++){
		proc->children[i] = NULL;
	}
	
	proc->waitpid_cv = cv_create("waitpid cv");
	if (proc->waitpid_cv == NULL){
		kfree(proc->children);
		p_filetable_destroy(proc->p_filetable);
		spinlock_cleanup(&proc->p_lock);
		kfree(proc->p_name);
		kfree(proc);
		return NULL;
	}
	proc->ops_lock = lock_create("PCB lock");
	if (proc->ops_lock == NULL){
	cv_destroy(proc->waitpid_cv);
	p_filetable_destroy(proc->p_filetable);
	spinlock_cleanup(&proc->p_lock);
	kfree(proc->p_name);
	kfree(proc);
	return NULL;

	}

	proc->waitpid_lock = lock_create("waitpid lock");
	if (proc->waitpid_lock == NULL){
		lock_destroy(proc->ops_lock);
		cv_destroy(proc->waitpid_cv);
		p_filetable_destroy(proc->p_filetable);
		spinlock_cleanup(&proc->p_lock);
		kfree(proc->p_name);
		kfree(proc);
		return NULL;
	}

	return proc;
}

/*
 * Destroy a proc structure.
 *
 * Note: nothing currently calls this. Your wait/exit code will
 * probably want to do so.
 */
void
proc_destroy(struct proc *proc)
{
	/*
	 * You probably want to destroy and null out much of the
	 * process (particularly the address space) at exit time if
	 * your wait/exit design calls for the process structure to
	 * hang around beyond process exit. Some wait/exit designs
	 * do, some don't.
	 */

	KASSERT(proc != NULL);
	KASSERT(proc != kproc);

	/*
	 * We don't take p_lock in here because we must have the only
	 * reference to this structure. (Otherwise it would be
	 * incorrect to destroy it.)
	 */

	/* VFS fields */
	if (proc->p_cwd) {
		VOP_DECREF(proc->p_cwd);
		proc->p_cwd = NULL;
	}

	/* VM fields */
	if (proc->p_addrspace) {
		/*
		 * If p is the current process, remove it safely from
		 * p_addrspace before destroying it. This makes sure
		 * we don't try to activate the address space while
		 * it's being destroyed.
		 *
		 * Also explicitly deactivate, because setting the
		 * address space to NULL won't necessarily do that.
		 *
		 * (When the address space is NULL, it means the
		 * process is kernel-only; in that case it is normally
		 * ok if the MMU and MMU- related data structures
		 * still refer to the address space of the last
		 * process that had one. Then you save work if that
		 * process is the next one to run, which isn't
		 * uncommon. However, here we're going to destroy the
		 * address space, so we need to make sure that nothing
		 * in the VM system still refers to it.)
		 *
		 * The call to as_deactivate() must come after we
		 * clear the address space, or a timer interrupt might
		 * reactivate the old address space again behind our
		 * back.
		 *
		 * If p is not the current process, still remove it
		 * from p_addrspace before destroying it as a
		 * precaution. Note that if p is not the current
		 * process, in order to be here p must either have
		 * never run (e.g. cleaning up after fork failed) or
		 * have finished running and exited. It is quite
		 * incorrect to destroy the proc structure of some
		 * random other process while it's still running...
		 */
		struct addrspace *as;

		kfree(proc->children);
		p_filetable_destroy(proc->p_filetable);

		if (proc == curproc) {
			as = proc_setas(NULL);
			as_deactivate();
		}
		else {
			as = proc->p_addrspace;
			proc->p_addrspace = NULL;
		}
		as_destroy(as);
	}

	KASSERT(proc->p_numthreads == 0);
	cv_destroy(proc->waitpid_cv);
	lock_destroy(proc->ops_lock);
	lock_destroy(proc->waitpid_lock);
	spinlock_cleanup(&proc->p_lock);

	kfree(proc->p_name);
	kfree(proc);
}



int get_next_pid(void){
	lock_acquire(global_ptable->proctable_lk);
	int toreturn = global_ptable->next_pid;
	global_ptable->next_pid++;
	lock_release(global_ptable->proctable_lk);
	return toreturn;
}

/*
 * Create a fresh proc for use by runprogram.
 *
 * It will have no address space and will inherit the current
 * process's (that is, the kernel menu's) current directory.
 */
struct proc *
proc_create_runprogram(const char *name)
{
	struct proc *newproc;

	newproc = proc_create(name);
	if (newproc == NULL) {
		return NULL;
	}
	
	

	/* VM fields */

	newproc->p_addrspace = NULL;

	/* VFS fields */

	/*
	 * Lock the current process to copy its current directory.
	 * (We don't need to lock the new process, though, as we have
	 * the only reference to it.)
	 */
	spinlock_acquire(&curproc->p_lock);
	if (curproc->p_cwd != NULL) {
		VOP_INCREF(curproc->p_cwd);
		newproc->p_cwd = curproc->p_cwd;
	}
	spinlock_release(&curproc->p_lock);

	return newproc;
}

/*
 * Add a thread to a process. Either the thread or the process might
 * or might not be current.
 *
 * Turn off interrupts on the local cpu while changing t_proc, in
 * case it's current, to protect against the as_activate call in
 * the timer interrupt context switch, and any other implicit uses
 * of "curproc".
 */
int
proc_addthread(struct proc *proc, struct thread *t)
{
	int spl;

	KASSERT(t->t_proc == NULL);

	spinlock_acquire(&proc->p_lock);
	proc->p_numthreads++;
	spinlock_release(&proc->p_lock);

	spl = splhigh();
	t->t_proc = proc;
	splx(spl);

	return 0;
}

/*
 * Remove a thread from its process. Either the thread or the process
 * might or might not be current.
 *
 * Turn off interrupts on the local cpu while changing t_proc, in
 * case it's current, to protect against the as_activate call in
 * the timer interrupt context switch, and any other implicit uses
 * of "curproc".
 */
void
proc_remthread(struct thread *t)
{
	struct proc *proc;
	int spl;

	proc = t->t_proc;
	KASSERT(proc != NULL);
	//kprintf("BEFORE SPINLOCK\n");
	spinlock_acquire(&proc->p_lock);
	//kprintf("IN SPINLOCK");
	KASSERT(proc->p_numthreads > 0);
	proc->p_numthreads--;
	//kprintf("NUMTHREADS");
	spinlock_release(&proc->p_lock);
	//kprintf("ALMOST DESTROYED");
	spl = splhigh();
	proc_destroy(t->t_proc);
	t->t_proc = NULL;
	splx(spl);
	//kprintf("DESTROYED");
}

/*
 * Fetch the address space of (the current) process.
 *
 * Caution: address spaces aren't refcounted. If you implement
 * multithreaded processes, make sure to set up a refcount scheme or
 * some other method to make this safe. Otherwise the returned address
 * space might disappear under you.
 */
struct addrspace *
proc_getas(void)
{
	struct addrspace *as;
	struct proc *proc = curproc;

	if (proc == NULL) {
		return NULL;
	}

	spinlock_acquire(&proc->p_lock);
	as = proc->p_addrspace;
	spinlock_release(&proc->p_lock);
	return as;
}

/*
 * Change the address space of (the current) process. Return the old
 * one for later restoration or disposal.
 */
struct addrspace *
proc_setas(struct addrspace *newas)
{
	struct addrspace *oldas;
	struct proc *proc = curproc;

	KASSERT(proc != NULL);

	spinlock_acquire(&proc->p_lock);
	oldas = proc->p_addrspace;
	proc->p_addrspace = newas;
	spinlock_release(&proc->p_lock);
	return oldas;
}

/*
  Create the process table structure
*/
struct proctable* proctable_create(void){
	struct proctable *toreturn;

	toreturn = kmalloc(sizeof(struct proctable));
	if (toreturn == NULL){
		return NULL;
	}

	toreturn->proc_array = kmalloc(sizeof(struct proc*) * __PROC_MAX);

	if (toreturn->proc_array == NULL){
		kfree(toreturn);
		return NULL;
	}

	toreturn->proctable_lk = lock_create("proctable_lock");
	if (toreturn->proctable_lk == NULL){
		kfree(toreturn->proc_array);
		kfree(toreturn);
		return NULL;
	}

	toreturn->waitpid_cv = cv_create("waitpid_cv");
	if (toreturn->waitpid_cv == NULL){
		lock_destroy(toreturn->proctable_lk);
		kfree(toreturn->proc_array);
		kfree(toreturn);
		return NULL;
	}

	for (int i = 0; i < __PROC_MAX; i++){
		toreturn->proc_array[i] = NULL;
	}

	toreturn->active_exit_codes = kmalloc(sizeof(struct pidxcode*) * __PROC_MAX);//rememver to check for fail
	if (toreturn->active_exit_codes == NULL){
		cv_destroy(toreturn->waitpid_cv);
		lock_destroy(toreturn->proctable_lk);
		kfree(toreturn->proc_array);
		kfree(toreturn);
		return NULL;
	}
	for (int i = 0; i < __PROC_MAX; i++){
		toreturn->active_exit_codes[i] = NULL;
	}

	toreturn->next_pid = __PID_MIN;
	toreturn->num_of_procs = 0;

	return toreturn;
}
/*
  Destroy the process table structure
*/
void proctable_destroy(struct proctable* pt){ //KASSERT
	kfree(pt->active_exit_codes);
	lock_destroy(pt->proctable_lk);
	cv_destroy(pt->waitpid_cv);
	kfree(pt->proc_array);
	kfree(pt);
}

/*
  put a process on the table
*/
int put_proc_on_proctable(struct proc* p){
	lock_acquire(global_ptable->proctable_lk);
	int i = 0;
	while(i < __PROC_MAX && global_ptable->proc_array[i] != NULL){
		i++;
	}
	if (i == __PROC_MAX){
		lock_release(global_ptable->proctable_lk);
		return -1;
	}
	
	global_ptable->proc_array[i] = p;
	global_ptable->num_of_procs++;
	lock_release(global_ptable->proctable_lk);
	return 0;
}

/* 
  Bootstrap function for the global process table
*/
void proctable_bootstrap(void){
	global_ptable = proctable_create();
	if (global_ptable == NULL){
		panic("Could not create global proc table!");
	}
}


/*
  remove a process from the process table
*/
void remove_proc_from_proctable(pid_t pid){
	lock_acquire(global_ptable->proctable_lk);
	int found_index = -1;
	for (int i = 0; i < __PROC_MAX; i++){
		if (global_ptable->proc_array[i] != NULL && pid == global_ptable->proc_array[i]->pid){
			found_index = i;
		}
	}

	if (found_index != -1){
		global_ptable->proc_array[found_index] = NULL;
		global_ptable->num_of_procs--;
	}

	lock_release(global_ptable->proctable_lk);

}

/*
  Get a pointer to a process in the table by its pid
*/
struct proc* get_reference_by_pid(pid_t pid){
	lock_acquire(global_ptable->proctable_lk);
	int found_index = -1;
	for (int i = 0; i < __PROC_MAX; i++){
		if (global_ptable->proc_array[i] != NULL ){

			if (pid == global_ptable->proc_array[i]->pid){
			found_index = i;
			}
		}
	}

	if (found_index != -1){
		lock_release(global_ptable->proctable_lk);
		return global_ptable->proc_array[found_index];
	}

	lock_release(global_ptable->proctable_lk);
	return NULL;
}

/*
  add a child to a parent process (unused)
*/
int add_child_to_proc(struct proc* parent, struct proc* child){
	lock_acquire(parent->ops_lock);

	bool was_found = 0;
	for (int i = 0; i < __PROC_CHILD_MAX; i++){//remember child goes on proc table
        if (parent->children[i] == NULL){
            parent->children[i] = child;
			was_found = 1;
            break; 
        }
    }

	lock_release(parent->ops_lock);
	return was_found;
}
/*
  remove all exit codes in the process table associated with this process
*/
void rem_all_exit_codes(pid_t pid){
	lock_acquire(global_ptable->proctable_lk);
	for (int i = 0; i < __PROC_MAX; i++){
            if (global_ptable->active_exit_codes[i] != NULL ){
                if (global_ptable->active_exit_codes[i]->ppid == pid){
                    kfree(global_ptable->active_exit_codes[i]);
                    global_ptable->active_exit_codes[i] = NULL;
                }
            }// removing all exit codes
        }

	lock_release(global_ptable->proctable_lk);
}

/*
  post an exit code to the process table
*/
void post_exit_code(int rawexcode, pid_t ppid, pid_t pid){
	lock_acquire(global_ptable->proctable_lk);
	struct pidxcode* excode = kmalloc(sizeof(struct pidxcode));
	if (excode != NULL){
		excode->ppid = ppid;
        excode->pid = pid;
        excode->exit_code = rawexcode;
	for (int i = 0; i < __PROC_MAX; i++){
            if (global_ptable->active_exit_codes[i] == NULL ){
                global_ptable->active_exit_codes[i] = excode;
                break;
            }// posting exit code
        }
	}
	lock_release(global_ptable->proctable_lk);
}

/*
  returns true if a process with pid searchpid has exited, false otherwise
*/
bool is_proc_exited(struct proctable *ptable, pid_t searchpid){
	lock_acquire(ptable->proctable_lk);
	for (int i = 0; i < __PROC_MAX; i++){
		if (ptable->active_exit_codes[i] != NULL && ptable->active_exit_codes[i]->pid == searchpid){
			lock_release(ptable->proctable_lk);
			return true;
		}
	}
	lock_release(ptable->proctable_lk);
	return false;
}

/*
  remove a child from a process (unused)
*/
void remove_child_from_proc(struct proc* remfrom, struct proc* rem){
	lock_acquire(remfrom->ops_lock);
	lock_acquire(rem->ops_lock);
	for (int i = 0; i < __PROC_MAX; i++){
		if (remfrom->children[i] != NULL && remfrom->children[i]->pid == rem->pid){
			 remfrom->children[i] = NULL;
		}
	}

	
	lock_release(rem->ops_lock);
	lock_release(remfrom->ops_lock);

}

/*
  returns true if a child is a child of another process (unused)
*/
bool is_proc_children(struct proc *proc, struct proc *other_proc){
	//if proc->ppid == other_proc->pid return true else false
	spinlock_acquire(&proc->p_lock);
	if (proc->ppid == other_proc->pid){
		spinlock_release(&proc->p_lock);
		return true;
	}
	spinlock_release(&proc->p_lock);
	return false;
	
}
/*
  read and remove an exit code from the table.
*/
int get_exit_code(struct proctable *ptable, pid_t ppid, pid_t pid){
	lock_acquire(ptable->proctable_lk);
	int returnval;
	for(int i = 0; i < __PROC_MAX; i++){
		if (ptable->active_exit_codes[i] != NULL && ptable->active_exit_codes[i]->ppid == ppid && ptable->active_exit_codes[i]->pid == pid){
			returnval = ptable->active_exit_codes[i]->exit_code;
			kfree(ptable->active_exit_codes[i]);
			ptable->active_exit_codes[i] = NULL;
			lock_release(ptable->proctable_lk);
			return returnval;
		}
	}
	lock_release(ptable->proctable_lk);
	return -1;
}

/*
 * Create the process structure for the kernel.
 */
void
proc_bootstrap(void)
{
	kproc = proc_create("[kernel]");
	if (kproc == NULL) {
		panic("proc_create for kproc failed\n");
	}
	//put_proc_on_proctable(kproc);
}




