#include <types.h>
#include <filetable.h>
#include <vnode.h>
#include <vfs.h>
#include <synch.h>
#include <lib.h>
#include <kern/limits.h>
#include <kern/unistd.h>
#include <lib.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <current.h>
/* global file table  structure */
struct g_filetable *global_ftable;
struct lock *fileops_lock;

struct file * create_file(char* path){//create a file structure and assign default values
    struct file* toreturn;

    

    toreturn = kmalloc(sizeof(struct file));//strdup name probably
    if (toreturn == NULL){
        return NULL;
    }
    

    toreturn->file_path = kstrdup(path);
    if (toreturn->file_path == NULL){
        kfree(toreturn);
        return NULL;
    }

    toreturn->seek_pos = kmalloc(sizeof(off_t));
    if (toreturn->seek_pos == NULL){
        kfree(toreturn->file_path);
        kfree(toreturn);
        return NULL;
    }

    toreturn->file_lock = lock_create("file lock");
    if (toreturn->file_lock == NULL){
        kfree((off_t*) toreturn->seek_pos);
        kfree(toreturn->file_path);
        kfree(toreturn);
        return NULL;
    }

    

    toreturn->file_vnode = NULL; 
    
    toreturn->is_dupe = 0;
    
    toreturn->refcount = 1; 
    *toreturn->seek_pos = 0;

    toreturn->can_read = 0;
    toreturn->can_write = 0;
    
    toreturn->global_fd = -1;
    toreturn->local_fd = -1;

    toreturn->std_file_status = -1;

    return toreturn;
}

/*
  create a standard file, this acts like a regular file but has no vnode or seek position
  and represents STDIN, STDOUT, and STDERR.
*/
struct file * create_std_file(int stdfd){
        struct file* stdfile;
	char name[7];
        if (stdfd == 0){
	    strcpy(name, "STDIN");
            stdfile = create_file(name);
        } 
        else if (stdfd == 1){
	    strcpy(name,"STDOUT");
            stdfile = create_file(name);
        }
        else {
	    strcpy(name, "STDERR");
            stdfile = create_file(name);
        }

        if (stdfile == NULL){
            return NULL;
        }

        kfree((off_t*)stdfile->seek_pos);
        stdfile->can_read = 1;
        stdfile->can_write = 1;
        stdfile->local_fd = stdfd;
        stdfile->seek_pos = NULL;
        stdfile->std_file_status = stdfd;

        return stdfile;

        
    }

/*
  Destroys a file
*/
void destroy_file(struct file* f){

    if (f != NULL)
    {
        VOP_DECREF(f->file_vnode); //should be VOP_DECREF
        kfree((off_t*)f->seek_pos);
        kfree(f->file_path);
        lock_destroy(f->file_lock);
        kfree(f);
    }

}

/*
  Destroys a file duplicate
*/
void destroy_dup(struct file* f){
    if (f != NULL){

    kfree(f->file_path);

    lock_destroy(f->file_lock);
    kfree(f);
    }
}

/*
  Copies a file
*/
struct file * copy_file(struct file* f){
    lock_acquire(f->file_lock);
    char* copyname = kstrdup(f->file_path);
    struct file * toreturn = create_file(copyname);

    toreturn->file_vnode = f->file_vnode;

    f->refcount++;
    toreturn->refcount = f->refcount;

    if (f->file_vnode != NULL){
        VOP_INCREF(f->file_vnode); //VOP_INCREF
    }
    if (f->seek_pos != NULL){
        *toreturn->seek_pos = *f->seek_pos;
    }
    toreturn->local_fd = f->local_fd;
    toreturn->global_fd = f->global_fd;
    toreturn->can_write = f->can_write;
    toreturn->can_read = f->can_read;
    toreturn->std_file_status = f->std_file_status;
    lock_release(f->file_lock);

    return toreturn;
}


/*
  Create the global file table
  ========================================================================
  NOTE:
  standard files for stdin, out, and err are created here.

  They will always exist and cannot be deleted.
*/
struct g_filetable *g_filetable_create(void){
    struct g_filetable *g_ft;
    g_ft = kmalloc(sizeof(struct g_filetable));
    if (g_ft == NULL){
        return NULL;} 
    g_ft->open_files = kmalloc(sizeof(struct file*) * __OPEN_MAX);
    if (g_ft->open_files == NULL){
               kfree(g_ft);
               return NULL;}
    g_ft->open_files_name = kmalloc(sizeof(char*) * __OPEN_MAX);
    if (g_ft->open_files_name == NULL){
        kfree(g_ft->open_files);
        kfree(g_ft);
        return NULL;
          }

    for (int i = 0; i < __OPEN_MAX; i++){
        g_ft->open_files[i] = NULL;
        g_ft->open_files_name[i] = NULL;
    }
    g_ft->open_files[0] = create_std_file(0);
    g_ft->open_files[1] = create_std_file(1);
    g_ft->open_files[2] = create_std_file(2);

    g_ft->next_fd = 3; //since STDIN_FILENO = 0, STDOUT_FILENO = 1, STDERR_FILENO = 2 so the next available fd is set to 3
    g_ft->size = 3;

    g_ft->global_file_lk = lock_create("global-ft-lock");
    if (g_ft->global_file_lk == NULL) {
        kfree(g_ft->open_files);
        kfree(g_ft->open_files_name);
        kfree(g_ft);
		return NULL;
	}
    return g_ft;
}
/*
  Destroy the global file table
*/
void g_filetable_destroy(struct g_filetable * g_ft){
    KASSERT(g_ft != NULL);
    KASSERT(g_ft->open_files != NULL);
    for (int i = 0; i < __OPEN_MAX; i++){
        destroy_file(g_ft->open_files[i]);
        kfree(g_ft->open_files_name[i]);
    }
    kfree(g_ft->open_files);
    kfree(g_ft->open_files_name);
    lock_destroy(g_ft->global_file_lk);
    kfree(g_ft);
}

/*
  Lookup a file on the global file table by its string name
*/
struct file *g_filetable_lookup(struct g_filetable * g_ft, char *file_name){
    lock_acquire(g_ft->global_file_lk);
    int file_index; 
    struct file *toreturn = NULL;
    //toreturn = kmalloc(sizeof(struct file));
    file_index = -1;
    if (file_name == NULL){
	lock_release(g_ft->global_file_lk);
        return NULL;
    }
    if (g_ft->size == 0){
       lock_release(g_ft->global_file_lk);
       return NULL;}
    for(int i = 0; i < __OPEN_MAX; i++){
	if (g_ft->open_files_name[i] != NULL){
        if(strcmp(g_ft->open_files_name[i], file_name) == 0){ 
            file_index = i;
        }}
    }
    if (file_index != -1){
    toreturn = g_ft->open_files[file_index];}
    lock_release(g_ft->global_file_lk);

    return toreturn;

}

/*
  bootstrap the global file table
*/
void global_table_bootstrap(void){
    global_ftable = g_filetable_create();
    fileops_lock = lock_create("file lock");
    if (global_ftable == NULL || fileops_lock == NULL){
        panic("Could not create global file table!");
    }
}

/*
  get the next available file descriptor
*/
int get_next_fd(void){
    lock_acquire(global_ftable->global_file_lk);
	int toreturn = global_ftable->next_fd;
	global_ftable->next_fd++;
	lock_release(global_ftable->global_file_lk);
	return toreturn;
}

/*
  put a file on the global file table
*/
int put_file_on_gtable(struct g_filetable * g_ft, char *file_name, struct file* f){
    lock_acquire(g_ft->global_file_lk);
    //toreturn = kmalloc(sizeof(struct file));
    int toreturn_fd = -1;
    if(g_ft->size < __OPEN_MAX){ //if there's room to add to the table
        for(int i = 0; i < __OPEN_MAX; i++){
            if(g_ft->open_files[i] == NULL){ //find the available space in the array to put file on table
                g_ft->open_files[i] = f;//remember in open to not use the copy directly
                g_ft->open_files_name[i] = file_name;
                g_ft->size++;
                break;
            }
        }
        toreturn_fd = f->global_fd;
    }
    
    lock_release(g_ft->global_file_lk);
    return toreturn_fd;
}


/*
  fully clear a file off of the global table
*/
int clear_file_from_gtable(struct g_filetable * g_ft, int fd){
    lock_acquire(g_ft->global_file_lk);
    // look up a file from the array given the fd and delete that from the array of open file and open file name
    int found_index = -1;
    int toreturn_fd = -1;
    int search_fd = fd;
    struct file *todestroy;
    todestroy = kmalloc(sizeof(struct file));
    for(int i = 0; i < __OPEN_MAX; i++){
	if (g_ft->open_files[i] != NULL){
        if(g_ft->open_files[i]->global_fd == search_fd){
            toreturn_fd = g_ft->open_files[i]->global_fd;
            found_index = i;
        }}
    }
    if(found_index != -1){
        todestroy = g_ft->open_files[found_index];
        g_ft->open_files[found_index] = NULL;
        g_ft->open_files_name[found_index] = NULL;
        destroy_file(todestroy);
        g_ft->size --;
    }

    lock_release(g_ft->global_file_lk);
    return toreturn_fd;



}

/*
  close a file on the global table, may or may not clear the file depending on ref count
*/
void close_file_on_gtable(struct g_filetable * g_ft, int fd){
    lock_acquire(g_ft->global_file_lk);
    int found_index = -1;
    int search_fd = fd;
    struct file *toreturn;
    struct file *tester;
    for(int i = 0; i < __OPEN_MAX; i++){
        tester = g_ft->open_files[i];
       
	if (tester != NULL){
    
        if(tester->global_fd == search_fd){
            found_index = i;
        }}
    }

    if(found_index != -1){
        toreturn = g_ft->open_files[found_index];
        if(toreturn->refcount == 1){
            //clear_file_from_gtable(g_ft, toreturn- >fd);
            g_ft->open_files[found_index] = NULL;
            g_ft->open_files_name[found_index] = NULL;
	  
            destroy_file(toreturn);
            g_ft->size --;
        }
        else{
        toreturn->refcount --;
        }
    }


    lock_release(g_ft->global_file_lk);
    
}

/*
  returns true if a file is opened, else false
*/
bool is_file_opened(struct g_filetable * g_ft, struct file *file){
    lock_acquire(g_ft->global_file_lk);
    struct file *f = g_filetable_lookup(g_ft, file->file_path);
    lock_release(g_ft->global_file_lk);
    return f != NULL;

}

/*
  reassign file's fd on global table (unused)
*/
void assign_file_fd_gtable(struct g_filetable * g_ft, struct file* f, int newfd){ //should be replaced by duplicate function, give different fd with same seek pos
    struct file *g_file = g_filetable_lookup(g_ft, f->file_path);
    g_file->global_fd = newfd;
    lock_acquire(g_file->file_lock);
    g_file->refcount++; //to keep track of file in g_ft for deallocating, different from file_vnode->ref_count
    vnode_incref(g_file->file_vnode);
    lock_release(g_file->file_lock);
}

/*p_filetable struct methods*/
/*
  create a per proc file table
========================================================
  NOTE:
  note that the first three files in every per proc file table are the three standard files
  in the global table. This is hardcoded and these files will always exist.
*/
struct p_filetable* p_filetable_create()
{
    struct p_filetable* p_ft;
    p_ft = kmalloc(sizeof(struct p_filetable));
        if (p_ft == NULL) {
            return NULL;
        }
    p_ft->proc_ft_lock = lock_create("proc ft lock");
        if (p_ft->proc_ft_lock == NULL){
            kfree(p_ft);
            return NULL;
        }
    p_ft->files = kmalloc(sizeof(struct file*) * __OPEN_MAX);  
    if (p_ft->files == NULL) {
        lock_destroy(p_ft->proc_ft_lock);
        kfree(p_ft);
        return NULL;
    }
    // default initialization of empty array to NULL
    for (int i = 0; i < __OPEN_MAX; i++) {
        p_ft->files[i] = NULL;
    }
//    lock_acquire(global_ftable->global_file_lk);
    p_ft->files[0] = global_ftable->open_files[0];
    p_ft->files[1] = global_ftable->open_files[1];
    p_ft->files[2] = global_ftable->open_files[2];
//    lock_release(global_ftable->global_file_lk);

    p_ft->stdin_open = true;
    p_ft->stdout_open = true;
    p_ft->stderr_open = true;
    p_ft->size = 0;

    return p_ft;
}

/*
  look up a file by fd on a per proc file table
*/
struct file* p_filetable_lookup(struct p_filetable* p_ft, int file_desc)
{
    lock_acquire(p_ft->proc_ft_lock);
    for (int i = 0; i < __OPEN_MAX; i++)
    {   if (p_ft->files[i] != NULL){
        if (p_ft->files[i]->local_fd == file_desc) {
            lock_release(p_ft->proc_ft_lock);
            return p_ft->files[i];
        }}
    }
    lock_release(p_ft->proc_ft_lock);
    return NULL;
}

/*
  put a file on the per proc file table
*/
int put_file_on_ptable(struct p_filetable* p_ft, struct file* f)
{
    lock_acquire(p_ft->proc_ft_lock);
    for (int i = 0; i < __OPEN_MAX; i++)
    {
        if (p_ft->files[i] == NULL) {
            p_ft->files[i] = f;
            p_ft->size++;
            lock_release(p_ft->proc_ft_lock);
            return f->local_fd;
        }
    }
    //p_ft->size++;
    lock_release(p_ft->proc_ft_lock);
    return -1;
}

/*
  close a file on per proc file table, may or may not close on global table due to dupe status
*/
void close_file_on_ptable(struct p_filetable * p_ft, int fd)
{
    //lock_acquire(p_ft->proc_ft_lock);
    for (int i = 0; i < __OPEN_MAX; i++)
    {   
        if (p_ft->files[i] != NULL){
        if (p_ft->files[i]->local_fd == fd) {
            if (p_ft->files[i]->is_dupe){
                destroy_dup(p_ft->files[i]);
                p_ft->files[i] = NULL;

            }
            else{
            close_file_on_gtable(global_ftable, p_ft->files[i]->global_fd);
            p_ft->files[i] = NULL;
            }
        }}
    }
    p_ft->size--;
    //lock_release(p_ft->proc_ft_lock);    
}

/*
  reassign file fd on local proc table (unused)
*/
void assign_file_fd(struct p_filetable *p_ft, struct file* f, int newfd){

    struct file *dup = dup_file(p_ft, f->local_fd, newfd);
    lock_acquire(dup->file_lock);
    dup->refcount++; //to keep track of file in p_ft for deallocating, different from file_vnode->ref_count
    dup->local_fd = newfd;
    vnode_incref(dup->file_vnode);//VOP_incref
    lock_release(dup->file_lock);

}

/*
  duplicate a file
*/
struct file* dup_file(struct p_filetable* dupefrom, int oldfd, int newfd){
    struct file* oldfile = p_filetable_lookup(dupefrom, oldfd);
    struct file* toreturn = copy_file(oldfile);
    kfree((off_t*)toreturn->seek_pos);
    toreturn->seek_pos = oldfile->seek_pos;
    toreturn->local_fd = newfd;
    toreturn->is_dupe = 1;
    
    return toreturn;

}

/*
  copy a per proc file table
*/
void copy_p_filetable(struct p_filetable* copyfrom, struct p_filetable* copyto){
    lock_acquire(copyfrom->proc_ft_lock);
    lock_acquire(copyto->proc_ft_lock);

    for (int i = 0; i < __OPEN_MAX; i++){//remember child goes on proc table
        if (copyfrom->files[i] != NULL){
            lock_acquire(copyfrom->files[i]->file_lock);
            if (copyfrom->files[i]->file_vnode != NULL){
                VOP_INCREF(copyfrom->files[i]->file_vnode);
               // copyfrom->files[i]->refcount++;
            }
		copyfrom->files[i]->refcount++;
            lock_release(copyfrom->files[i]->file_lock);
            copyto->files[i] = copyfrom->files[i];
            
        }
    }
    lock_release(copyto->proc_ft_lock);
    lock_release(copyfrom->proc_ft_lock);

}

/*
  close all files on a per proc file table (hacky but works)
*/
void close_all_files_ptable(struct p_filetable* closeon){
    lock_acquire(closeon->proc_ft_lock);
    for (int i = 0; i < __OPEN_MAX; i++){
       
        if (closeon->files[i] != NULL){
            lock_acquire(closeon->files[i]->file_lock);
            if(closeon->files[i]->std_file_status == -1){
            if (closeon->files[i]->file_vnode != NULL){
                if (closeon->files[i]->refcount > 1){
                    VOP_DECREF(closeon->files[i]->file_vnode);
                    closeon->files[i]->refcount--;
                    lock_release(closeon->files[i]->file_lock);
                    closeon->files[i] = NULL;

                }else {
                    lock_release(closeon->files[i]->file_lock);
                    close_file_on_gtable(global_ftable, closeon->files[i]->global_fd);
                    closeon->files[i] = NULL;
                }
            }else{
		if (closeon->files[i]->refcount > 1){
			closeon->files[i]->refcount--;
			lock_release(closeon->files[i]->file_lock);
			closeon->files[i] = NULL;}
		else{
                lock_release(closeon->files[i]->file_lock);
		        destroy_dup(closeon->files[i]);
		        closeon->files[i] = NULL;}
            }
            //lock_release(copyfrom->files[i]->file_lock);
            }
            else{
                lock_release(closeon->files[i]->file_lock);
            }

        }
    
    closeon->size = 0;
    }
    lock_release(closeon->proc_ft_lock);
}

/*
  destroy a per proc file table
*/
void p_filetable_destroy(struct p_filetable* p_ft)
{
    close_all_files_ptable(p_ft);

    lock_destroy(p_ft->proc_ft_lock);

    kfree(p_ft);
}

