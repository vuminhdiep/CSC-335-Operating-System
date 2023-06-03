#include <types.h>
#include <vnode.h>
#include <synch.h>
#include <limits.h>

#ifndef _FILETABLE_H_
#define _FILETABLE_H_
/**
 * Structure for file.
 * This structure is to defined a file in the file system.
 * 
 */
struct file{
	volatile int refcount; //reference count of this file
	volatile off_t *seek_pos;     /* seek position of this file */

	struct lock *file_lock;   //lock for the file
	struct vnode *file_vnode;/* vnode for connecting to the file system */
	char* file_path;       /* file path of this file struct */
	int global_fd;                /* global table file descriptor */ //fd used in gtable, 
	volatile int local_fd;                /* local table file descriptor */ //fd used in ptable, not pointing to the same thing as fd in gtable
	volatile bool can_read; //flag for being able to read
	volatile bool can_write; //flag for being able to write
	volatile bool is_dupe;  //flag for if this file is a dupe or not
	int std_file_status; /* if -1, this is not a standard file. If anything other than -1, this is the standard file descriptor.*/

};

extern struct lock *fileops_lock; //globally lock

struct file * create_file(char* path); //create a file
struct file * create_std_file(int stdfd); //create a standard file
void destroy_file(struct file* f); //destroy a file
struct file * copy_file(struct file* f); //copy a file
void destroy_dup(struct file* f); //destroy a dupe

/**
 * Structure for per process file table.
 * This structure is used to keep track of all the files opened by a process.
 * 
 */
struct p_filetable { /* per process file table*/
	struct lock *proc_ft_lock; /* lock for the per process file table */
	struct file **files; /* array of pointers to file structs*/
	bool stdin_open;
	bool stdout_open;
	bool stderr_open;
	volatile int size;

};
struct file* dup_file(struct p_filetable* dupefrom, int oldfd, int newfd);
//duplicate a file

/*creates a filetable of size __OPEN_MAX*/
struct p_filetable* p_filetable_create(void); 
/*deallocates a filetable from memory*/
void p_filetable_destroy(struct p_filetable* p_ft);
/*looks up a file inside the per proc filetable based on a file's file description
returns null if the file cannot be found*/
struct file* p_filetable_lookup(struct p_filetable* p_ft, int file_desc);
// 
int put_file_on_ptable(struct p_filetable * p_ft, struct file* f); /* puts a file on the file table if there is enough room*/
/* returns the fd of the added file on success, -1 on fail*/

void close_file_on_ptable(struct p_filetable * p_ft, int fd); /* closes one reference to the file, removes from this
file table and alerts the global file table of the closure.*/

void copy_p_filetable(struct p_filetable* copyfrom, struct p_filetable* copyto);

void assign_file_fd(struct p_filetable * p_ft, struct file* f, int newfd); /*Assign a file already existed in p_filetable with a new fd*/

// bool is_file_opened_ptable(struct p_filetable * p_ft, int *fd); /*Check if a file is opened in the p_filetable*/
// kmalloc(sizeof(p_filetable * array_size))

/**
 * Structure for global file table.
 * This structure is used to keep track of all the files opened by all processes in the system.
 * 
 */
struct g_filetable{
	volatile int next_fd;                 /* variable to hold next file descriptor*/
	struct lock *global_file_lk; /* lock for the global file table */
	struct file** open_files; /* pointer to an array of open files */
	char** open_files_name;
	volatile int size;

	

};

/*Create a global opened file table*/
struct g_filetable *g_filetable_create(void);

/*Destroy the global file table*/
void g_filetable_destroy(struct g_filetable * g_ft);

/*Look up an opened file by name and return the file*/
struct file *g_filetable_lookup(struct g_filetable * g_ft, char *file_name);

void global_table_bootstrap(void); //bootstrapper for global file table

int get_next_fd(void);             /* increments and returns the next fd value in the global table*/

int put_file_on_gtable(struct g_filetable * g_ft, char *file_name, struct file* f); /* puts a file on the file table if there is enough room*/
/* returns the fd of the added file on success, -1 on fail*/

int clear_file_from_gtable(struct g_filetable * g_ft, int fd); /* deletes file from global file table, setting the places for name and the file pointer to NULL.*/
/* returns the fd of the file removed on success and -1 on fail, destroy file also*/

void close_file_on_gtable(struct g_filetable * g_ft, int fd); /* closes one reference to the file, detects if
															number of references will be 0 and deletes the file from the table ahead of time.*/
bool is_file_opened(struct g_filetable * g_ft, struct file *f); /*Check if the file is opened or not in global table*/

void assign_file_fd_gtable(struct g_filetable * g_ft, struct file* f, int newfd); /*Assign a file already existed in p_filetable with a new fd*/

extern struct g_filetable* global_ftable; //make the global table externally accessible.

void close_all_files_ptable(struct p_filetable* closeon); 
//automatically close all files in a per proc file table.



#endif /* _FILETABLE_H_ */
